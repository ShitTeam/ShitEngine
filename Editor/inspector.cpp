#include "inspector.h"

#include "assetpaths.h"
#include "componentmenu.h"
#include "systemmenu.h"
#include "dnd.h"
#include "pathfieldwidget.h"
#include "animatoreditorwidget.h"
#include "animatorwidget.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Component/AnimationComponent.h>
#include <ShitEngine/Animation/Animator.h>
#include <ShitEngine/System/System.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QDoubleSpinBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTabWidget>
#include <QUrl>
#include <QToolButton>
#include <QVBoxLayout>

#include <cstring>
#include <limits>
#include <typeindex>
#include <utility>

namespace {

using FieldMeta = Shit::FieldMeta;

/// 取字段的编辑元数据（meta 可多个，用第一个非空 displayName 的，否则空）
const FieldMeta* metaOf(const Shit::FieldInfo &field)
{
    return field.meta.empty() ? nullptr : &field.meta[0];
}

/// 类型名归一化：引擎头内声明为 "Vector2"，插件全局类引用为 "Shit::Vector2"
QString typeNameOf(const Shit::FieldInfo &field)
{
    const std::string &t = field.typeName;
    const std::string prefix = "Shit::";
    if (t.rfind(prefix, 0) == 0)
        return QString::fromStdString(t.substr(prefix.size()));
    return QString::fromStdString(t);
}

QString displayNameOf(const Shit::FieldInfo &field)
{
    if (const FieldMeta *m = metaOf(field); m && !m->displayName.empty())
        return QString::fromStdString(m->displayName);
    return QString::fromStdString(field.name);
}

/// 通用浮点编辑框（Vector2 分量用）
QDoubleSpinBox *makeSpin(float value)
{
    auto *box = new QDoubleSpinBox;
    box->setRange(-1e6, 1e6);
    box->setSingleStep(0.1);
    box->setDecimals(3);
    box->setValue(value);
    return box;
}

// ═══════════════════════════════════════════════════════════════
// 组件引用字段（ComponentRef<T>）——拖拽赋引用
// ═══════════════════════════════════════════════════════════════

/// 类型名归一化（去 "Shit::" 前缀；扫描器 refType 与 TypeRegistry 注册名均无前缀，防御处理）
QString normalizeTypeName(const std::string &name)
{
    QString q = QString::fromStdString(name);
    return q.remove("Shit::");
}

/// component 能否赋给 refType 引用字段（沿反射基类链向上查找，与运行期 dynamic_cast 语义一致）
bool isAssignable(const Shit::Component *component, const std::string &refType)
{
    const QString target = normalizeTypeName(refType);
    const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*component)));
    for (; ti; ti = ti->baseType) {
        if (normalizeTypeName(ti->name) == target) return true;
    }
    return false;
}

/// 组件标题：可拖拽（按住拖动到任意引用字段赋引用）
class ComponentHeaderLabel : public QLabel
{
public:
    ComponentHeaderLabel(const QString &text, const QByteArray &dragData, QWidget *parent)
        : QLabel(text, parent)
        , m_dragData(dragData)
    {
        setStyleSheet("font-weight:bold; color:#2a7ab1; margin-top:6px;");
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            m_pressPos = event->pos();
        QLabel::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if ((event->buttons() & Qt::LeftButton)
            && (event->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()) {
            auto *drag = new QDrag(this);
            auto *mime = new QMimeData;
            mime->setData(QString::fromLatin1(kDndComponentRef), m_dragData);
            drag->setMimeData(mime);
            drag->exec(Qt::CopyAction);
        }
        QLabel::mouseMoveEvent(event);
    }

private:
    QPoint m_pressPos;
    QByteArray m_dragData;
};

/// 组件引用字段的编辑控件：显示当前引用目标 + 拖放目标 + 清除按钮
class RefFieldWidget : public QWidget
{
public:
    using OnChanged = std::function<void()>;

    RefFieldWidget(Shit::Component *obj, const Shit::FieldInfo &field, OnChanged onChanged, QWidget *parent)
        : QWidget(parent)
        , m_obj(obj)
        , m_field(field)
        , m_scene(obj && obj->getOwner() ? obj->getOwner()->getScene() : nullptr)
        , m_onChanged(std::move(onChanged))
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        m_label = new QLabel(this);
        m_label->setFrameStyle(QFrame::StyledPanel);
        m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_label->setMinimumHeight(22);
        m_label->setToolTip(tr("把检查器里的组件标题、或场景树中的对象拖到这里赋引用"));

        m_clear = new QToolButton(this);
        m_clear->setText(tr("✕"));
        m_clear->setToolTip(tr("清除引用"));

        layout->addWidget(m_label, 1);
        layout->addWidget(m_clear);

        setAcceptDrops(true);

        connect(m_clear, &QToolButton::clicked, this, [this] {
            setFieldUuid(0);
            refresh();
            m_onChanged();
        });

        refresh();
    }

    /// 引擎 → 控件回读（检查器每帧 refresh 调用；目标销毁后自动显示 None）
    void refresh()
    {
        const uint64_t uuid = fieldUuid();
        const QString type = QString::fromStdString(m_field.refType);
        if (!m_scene || uuid == 0) {
            m_label->setText(tr("None (%1)").arg(type));
            return;
        }
        Shit::Component *target = m_scene->componentByUuid(uuid);
        if (!target) {
            m_label->setText(tr("None (%1)").arg(type));   // 目标已销毁/跨场景
            return;
        }
        const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*target)));
        const QString typeName = ti ? normalizeTypeName(ti->name) : tr("?");
        const QString ownerName = target->getOwner()
            ? QString::fromStdString(target->getOwner()->getName()) : tr("?");
        m_label->setText(QString("%1 (%2)").arg(ownerName, typeName));
    }

    /// 只读模式：隐藏清除按钮、拒绝拖放（readOnly 元数据的引用字段）
    void setReadOnly(bool readOnly)
    {
        m_clear->setVisible(!readOnly);
        setAcceptDrops(!readOnly);
    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (resolveDrop(event->mimeData()))
            event->acceptProposedAction();
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (resolveDrop(event->mimeData()))
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent *event) override
    {
        if (Shit::Component *target = resolveDrop(event->mimeData())) {
            setFieldUuid(target->getUuid());
            refresh();
            m_onChanged();
            event->acceptProposedAction();
        }
    }

private:
    uint64_t fieldUuid() const
    {
        uint64_t uuid = 0;
        std::memcpy(&uuid, m_field.GetFieldPtr(m_obj), sizeof(uuid));
        return uuid;
    }

    void setFieldUuid(uint64_t uuid)
    {
        std::memcpy(m_field.GetFieldPtr(m_obj), &uuid, sizeof(uuid));
    }

    /// 从拖拽数据解析出可赋值的组件（类型不符/跨场景/空返回 nullptr）
    Shit::Component *resolveDrop(const QMimeData *mime) const
    {
        if (!mime || !m_scene || !mime->hasFormat(QString::fromLatin1(kDndComponentRef)))
            return nullptr;
        const auto items = decodeComponentRefs(mime->data(QString::fromLatin1(kDndComponentRef)));
        for (const auto &item : items) {
            const quint64 uuid = item.first;
            Shit::Component *comp = m_scene->componentByUuid(uuid);
            if (comp && isAssignable(comp, m_field.refType))
                return comp;
        }
        return nullptr;
    }

    Shit::Component *m_obj;
    Shit::FieldInfo m_field;
    Shit::Scene *m_scene;
    OnChanged m_onChanged;
    QLabel *m_label;
    QToolButton *m_clear;
};

} // namespace

Inspector::Inspector(QWidget *parent)
    : QWidget(parent)
    , m_tabs(new QTabWidget(this))
    , m_scroll(new QScrollArea(this))
    , m_content(new QWidget)
    , m_form(new QFormLayout(m_content))
    , m_systemScroll(new QScrollArea(this))
    , m_systemContent(new QWidget)
    , m_systemPageLayout(new QVBoxLayout(m_systemContent))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    layout->addWidget(m_tabs);
    m_tabs->setDocumentMode(true);

    // 文件拖到属性面板 → 自动填充匹配的路径字段（P31，拖拽悬停时虚线高亮）
    setAcceptDrops(true);
    setStyleSheet("Inspector[dropActive=\"true\"] { border: 2px dashed #7ac0ff; }");

    // 页 1「组件」：搜索框 + 对象属性（启用/名称/Tag）+ 各组件字段 + Add Component
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索组件或字段…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #3a4a5a; border-radius: 3px;"
        "             background: #1c2430; color: #e0e8f0; padding: 3px 6px; }"
        "QLineEdit:focus { border-color: #7ac0ff; }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] { applyFilter(); });

    auto *componentPage = new QWidget(this);
    auto *componentPageLayout = new QVBoxLayout(componentPage);
    componentPageLayout->setContentsMargins(0, 0, 0, 0);
    componentPageLayout->setSpacing(2);
    componentPageLayout->addWidget(m_searchEdit);
    componentPageLayout->addWidget(m_scroll);

    m_scroll->setWidget(m_content);
    m_scroll->setWidgetResizable(true);
    m_form->setContentsMargins(8, 8, 8, 8);
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_tabs->addTab(componentPage, tr("组件"));

    // 页 2「系统」：场景系统列表 + 系统属性（常驻，未选中对象也能查看/编辑）
    m_systemPageLayout->setContentsMargins(8, 8, 8, 8);
    m_systemPageLayout->setSpacing(2);
    m_systemScroll->setWidget(m_systemContent);
    m_systemScroll->setWidgetResizable(true);
    m_tabs->addTab(m_systemScroll, tr("系统"));

    clear();
}

void Inspector::clear()
{
    m_readbacks.clear();
    m_sections.clear();   // 组件行区间随表单重建失效
    m_object = nullptr;
    // 系统页常驻独立：此处只清组件页，不动 m_selectedSystemName / 系统面板
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    auto *placeholder = new QLabel(tr("未选中对象\n\n选中场景中的对象后，\n在此编辑其组件属性。"), m_content);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    m_form->addRow(placeholder);
}

void Inspector::refresh()
{
    // 系统页刷新：比较签名，仅变化时重建。
    // 无条件检测（不依赖未选中态）——选中对象时系统列表也会变（如播放态物理自愈注册）
    if (m_scene) {
        std::string sig;
        for (const auto& name : m_scene->getRegisteredSystemTypeNames()) {
            auto* sys = m_scene->getSystem(name);
            if (!sys) continue;  // 系统可能在延迟移除队列中，跳过防空指针
            sig += name + ":" + std::to_string(sys->getPriority()) + ";";
        }
        if (sig != m_systemsSignature) {
            m_systemsSignature = sig;
            rebuildSystemPanel();
        }
    }
    for (auto &readback : m_readbacks)
        readback();
    for (auto &readback : m_systemReadbacks)
        readback();
}

void Inspector::setGameObject(Shit::GameObject *object)
{
    clear();
    m_object = object;
    m_componentCount = 0;
    m_fieldCount = 0;
    if (!object)
        return;

    // 组件标题之后继续添加行，故移除占位
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    m_tabs->setCurrentWidget(m_scroll);   // 选中对象 → 自动切到组件页（系统页随时可手动切回）

    // Unity 风格：顶部对象属性区 —— [启用 ✓][名称编辑框]，下方 Tag 行。
    // 名称提交走撤销"重命名"（与树内 F2 等价，两处同步）；启用/Tag 走通用字段撤销
    auto *nameRow = new QWidget(m_content);
    auto *nameRowLayout = new QHBoxLayout(nameRow);
    nameRowLayout->setContentsMargins(0, 0, 0, 0);
    nameRowLayout->setSpacing(4);

    auto *activeCheck = new QCheckBox(nameRow);
    activeCheck->setChecked(m_object->isActive());
    activeCheck->setToolTip(tr("启用/禁用对象（失活不渲染、不更新行为/UI/物理，子对象随父级联）"));
    connect(activeCheck, &QCheckBox::toggled, this, [this](bool on) {
        emit fieldEdited();               // undo begin（须在修改前）
        m_object->setActive(on);
        emit fieldCommitted();             // undo commit
    });
    m_readbacks.push_back([this, activeCheck] {
        activeCheck->blockSignals(true);
        activeCheck->setChecked(m_object->isActive());
        activeCheck->blockSignals(false);
    });
    nameRowLayout->addWidget(activeCheck);

    auto *nameEdit = new QLineEdit(nameRow);
    nameEdit->setAlignment(Qt::AlignCenter);
    nameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #3a4a5a; border-radius: 3px; background: #1c2430;"
        "            color: #e0e8f0; font-size: 14px; font-weight: bold; padding: 5px; }"
        "QLineEdit:focus { border-color: #7ac0ff; }");
    connect(nameEdit, &QLineEdit::editingFinished, this, [this, nameEdit] {
        const std::string newName = nameEdit->text().trimmed().toStdString();
        if (newName.empty() || newName == m_object->getName()) {
            nameEdit->setText(QString::fromStdString(m_object->getName()));   // 空/未变 → 还原显示
            return;
        }
        emit fieldEdited();               // undo begin + dirty（须在修改前）
        m_object->setName(newName);
        emit objectRenamed();             // undo commit（标签"重命名"）
    });
    m_readbacks.push_back([this, nameEdit] {
        // 播放中游戏逻辑改名 → 同步回显（blockSignals 防回显触发重命名提交）
        nameEdit->blockSignals(true);
        nameEdit->setText(QString::fromStdString(m_object->getName()));
        nameEdit->blockSignals(false);
    });
    nameRowLayout->addWidget(nameEdit, 1);
    m_form->addRow(nameRow);

    auto *tagEdit = new QLineEdit(QString::fromStdString(m_object->getTag()), m_content);
    tagEdit->setPlaceholderText(tr("Tag（如 player / enemy）"));
    tagEdit->setToolTip(tr("对象标签（用于分类查找，随场景保存）"));
    connect(tagEdit, &QLineEdit::editingFinished, this, [this, tagEdit] {
        const std::string newTag = tagEdit->text().toStdString();
        if (newTag == m_object->getTag()) return;
        emit fieldEdited();               // undo begin（须在修改前）
        m_object->setTag(newTag);
        emit fieldCommitted();             // undo commit
    });
    m_readbacks.push_back([this, tagEdit] {
        tagEdit->blockSignals(true);
        tagEdit->setText(QString::fromStdString(m_object->getTag()));
        tagEdit->blockSignals(false);
    });
    m_form->addRow(tr("Tag"), tagEdit);

    auto *nameSep = new QFrame(m_content);
    nameSep->setFrameShape(QFrame::HLine);
    nameSep->setStyleSheet("color: #2a3540;");
    m_form->addRow(nameSep);

    object->forEachComponent([this](Shit::Component *component) {
        if (!component) return;

        const Shit::TypeInfo *typeInfo = Shit::TypeRegistry::Get(std::type_index(typeid(*component)));
        if (!typeInfo || typeInfo->fields.empty())
            return; // 无反射元数据（如编辑器自定义 Behavior），跳过

        ++m_componentCount;
        // 记录本组件渲染的表单行区间 + 搜索键（P34：按区间整块过滤）
        const int startRow = m_form->rowCount();
        QString searchKey = normalizeTypeName(typeInfo->name);
        // 组件头可拖拽（携带自身 uuid + 类型名，供引用字段拖入）
        QList<std::pair<quint64, QString>> refs;
        refs.append({ component->getUuid(), QString::fromStdString(typeInfo->name) });
        auto *title = new ComponentHeaderLabel(QString::fromStdString(typeInfo->name),
                                               encodeComponentRefs(refs), m_content);

        // Unity 风格：组件头右侧「✕ 移除组件」（Transform 是引擎基础设施，不显示按钮）
        auto *row = new QWidget(m_content);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(0);
        rowLayout->addWidget(title, 1);
        if (normalizeTypeName(typeInfo->name) != "TransformComponent") {
            auto *removeBtn = new QToolButton(row);
            removeBtn->setText(tr("✕"));
            removeBtn->setToolTip(tr("移除组件"));
            removeBtn->setCursor(Qt::PointingHandCursor);
            removeBtn->setStyleSheet(
                "QToolButton { border: none; color: #7a8a9a; padding: 2px 6px; }"
                "QToolButton:hover { color: #ff6b6b; background: rgba(255, 80, 80, 0.12); }");
            rowLayout->addWidget(removeBtn);
            connect(removeBtn, &QToolButton::clicked, this,
                    [this, component] { removeComponentFromObject(component); });
        }
        m_form->addRow(row);

        // AnimationComponent 渲染入口控件——「打开 Animation 窗口」按钮，
        // 帧动画编辑统一收敛到 AnimationDock（跳过反射字段：m_clipsData 只读载体）
        if (normalizeTypeName(typeInfo->name) == "AnimationComponent") {
            auto *animator = new AnimatorEditorWidget(static_cast<Shit::AnimationComponent *>(component), m_content);
            // 包装成整行（不占 label 列）
            auto *animRow = new QWidget(m_content);
            auto *animLayout = new QVBoxLayout(animRow);
            animLayout->setContentsMargins(0, 0, 0, 0);
            animLayout->addWidget(animator);
            m_form->addRow(animRow);
            m_readbacks.push_back([animator] { animator->refresh(); });
            // 入口按钮 → 请求打开独立帧动画窗口
            connect(animator, &AnimatorEditorWidget::openEditorRequested, this,
                    &Inspector::openAnimationEditorRequested);
        }
        // Animator 渲染入口控件——「打开 Animator 窗口」按钮，编辑统一收敛到 AnimatorDock
        // （跳过反射字段：m_animatorData 序列化载体，避免与状态机编辑逻辑冲突）
        else if (normalizeTypeName(typeInfo->name) == "Animator") {
            auto *animator = new AnimatorWidget(static_cast<Shit::Animator *>(component), m_content);
            auto *animRow = new QWidget(m_content);
            auto *animLayout = new QVBoxLayout(animRow);
            animLayout->setContentsMargins(0, 0, 0, 0);
            animLayout->addWidget(animator);
            m_form->addRow(animRow);
            m_readbacks.push_back([animator] { animator->refresh(); });
            // 入口按钮 → 请求打开独立状态机窗口（状态机编辑统一收敛到 AnimatorDock）
            connect(animator, &AnimatorWidget::openEditorRequested, this,
                    &Inspector::openAnimatorEditorRequested);
        }
        else {
            for (const Shit::FieldInfo &field : typeInfo->fields) {
                addFieldRow(field, component);
                ++m_fieldCount;
            }
        }

        // 渲染属性（getter/setter 对）
        for (const auto &prop : typeInfo->properties) {
            addPropertyRow(prop, component);
            ++m_fieldCount;
        }

        for (const auto &f : typeInfo->fields)
            searchKey += " " + QString::fromStdString(f.name);
        m_sections.push_back({ startRow, m_form->rowCount() - 1, searchKey });
    });

    // Unity 风格 "Add Component"：组件列表底部的虚线按钮，点击弹出组件类型菜单
    auto *addBtn = new QPushButton(tr("Add Component"), m_content);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { border: 1px dashed #5a6a7a; border-radius: 3px; color: #9ab0c4;"
        "              background: transparent; padding: 5px; }"
        "QPushButton:hover { border-color: #7ac0ff; color: #d0e4f5; background: rgba(122,192,255,0.08); }");
    auto *row = new QWidget(m_content);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 4, 0, 0);
    rowLayout->addStretch(1);
    rowLayout->addWidget(addBtn);
    rowLayout->addStretch(1);
    m_form->addRow(row);
    connect(addBtn, &QPushButton::clicked, this, &Inspector::showAddComponentMenu);

    applyEditLock();   // 播放态编辑锁：重建后按当前播放状态重新禁用
}

void Inspector::setPlayMode(bool playing)
{
    m_playMode = playing;
    applyEditLock();
}

void Inspector::applyEditLock()
{
    // 两页内容分别统一递归禁用（含字段控件/移除按钮/Add Component/对象名栏/引用控件）；
    // QTabWidget 本体不禁用——播放态仍可切页查看运行时值（Unity 语义）。
    // 禁用控件仍可被回读刷新 setText/setValue（blockSignals 已防回环），运行时值实时可见
    if (m_content) m_content->setEnabled(!m_playMode);
    if (m_systemContent) m_systemContent->setEnabled(!m_playMode);
}

void Inspector::setScene(Shit::Scene *scene)
{
    // 同一场景的代数变化（对象增删/改名等）不影响系统列表，不重建系统页
    // （系统列表增删/优先级变化由 refresh() 的签名检测处理）
    if (m_scene == scene) return;
    m_scene = scene;
    m_systemsSignature.clear();
    m_selectedSystemName.clear();   // 场景已整体替换，旧展开的系统名不再有效
    rebuildSystemPanel();
}

// ═══════════════════════════════════════════════════════════════
// 文件拖到属性面板 → 自动填充匹配的路径字段（面板级兜底；
// 具体字段行上的 PathFieldWidget 接受更精确的字段级拖放）
// ═══════════════════════════════════════════════════════════════

bool Inspector::findFileDropTarget(const QString &filePath, Shit::Component **outComp,
                                   Shit::FieldInfo *outField) const
{
    if (!m_object) return false;
    if (m_playMode) return false;   // 播放态只读锁：不接受拖拽填充

    const QString ext = QFileInfo(filePath).suffix().toLower();
    // 全部已知资产后缀（用于"未知扩展 + 唯一候选"兜底判定）
    static const QStringList kAllKnown = {
        "png", "jpg", "jpeg", "bmp", "webp", "gif",
        "wav", "ogg", "mp3", "flac",
        "ttf", "otf", "ttc",
        "anim", "scene", "prefab",
    };

    std::vector<std::pair<Shit::Component *, const Shit::FieldInfo *>> stringFields;
    Shit::Component *hitComp = nullptr;
    const Shit::FieldInfo *hitField = nullptr;

    m_object->forEachComponent([&](Shit::Component *comp) {
        const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*comp)));
        if (!ti) return;
        for (const auto &field : ti->fields) {
            if (typeNameOf(field) != "std::string") continue;
            if (const FieldMeta *m = metaOf(field); m && m->readOnly) continue;
            stringFields.emplace_back(comp, &field);
            // 字段语义（图片/音频/字体/动画/场景/预置体）与拖入扩展名匹配
            const PathFieldSpec spec = pathSpecForFieldName(field.name);
            if (spec.isPath && spec.suffixes.contains(ext)) {
                hitComp = comp;
                hitField = &field;
            }
        }
    });
    if (hitComp) {
        if (outComp) *outComp = hitComp;
        if (outField) *outField = *hitField;
        return true;
    }

    // 兜底：扩展名不在任何已知语义表 && 对象只有一个可写 string 字段 → 视为目标
    //（如自定义路径字段），避免拖入被静默拒绝
    if (!kAllKnown.contains(ext) && stringFields.size() == 1) {
        if (outComp) *outComp = stringFields[0].first;
        if (outField) *outField = *stringFields[0].second;
        return true;
    }
    return false;
}

void Inspector::setDropActive(bool on)
{
    if (m_dropActive == on) return;
    m_dropActive = on;
    setProperty("dropActive", on);
    style()->unpolish(this);
    style()->polish(this);
}

void Inspector::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile() && findFileDropTarget(url.toLocalFile(), nullptr, nullptr)) {
                event->acceptProposedAction();
                setDropActive(true);
                return;
            }
        }
    }
    QWidget::dragEnterEvent(event);
}

void Inspector::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile() && findFileDropTarget(url.toLocalFile(), nullptr, nullptr)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    QWidget::dragMoveEvent(event);
}

void Inspector::dragLeaveEvent(QDragLeaveEvent *event)
{
    setDropActive(false);
    QWidget::dragLeaveEvent(event);
}

void Inspector::dropEvent(QDropEvent *event)
{
    setDropActive(false);
    if (!event->mimeData()->hasUrls()) {
        QWidget::dropEvent(event);
        return;
    }

    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
        Shit::Component *comp = nullptr;
        Shit::FieldInfo field;
        if (!findFileDropTarget(path, &comp, &field)) continue;

        // 与检查器其他编辑一致：先撤销 begin，写回规范化（项目内相对）路径，再提交
        emit fieldEdited();
        *reinterpret_cast<std::string *>(field.GetFieldPtr(comp)) =
            AssetPaths::toRelative(path).toStdString();
        comp->onFieldChanged(field.name);
        emit fieldCommitted();

        refresh();   // 回读更新对应控件显示
        ST_INFO("[Inspector] 文件拖入 {} -> {}.{}",
                AssetPaths::toRelative(path).toStdString(),
                comp->getOwner() ? comp->getOwner()->getName() : "?", field.name);
        event->acceptProposedAction();
        return;
    }
    QWidget::dropEvent(event);
}

void Inspector::applyFilter()
{
    const QString text = m_searchEdit->text().trimmed();

    // 按组件区间显隐：行上的 label/field/spanning 控件统一 setVisible。
    // 清空搜索框 = 全显（区间外行：对象属性区/Add Component 按钮始终显示）
    for (const auto &sec : m_sections) {
        const bool show = text.isEmpty() || sec.searchKey.contains(text, Qt::CaseInsensitive);
        for (int r = sec.startRow; r <= sec.endRow; ++r) {
            for (QFormLayout::ItemRole role : { QFormLayout::LabelRole,
                                                QFormLayout::FieldRole,
                                                QFormLayout::SpanningRole }) {
                if (auto *item = m_form->itemAt(r, role))
                    if (auto *w = item->widget())
                        w->setVisible(show);
            }
        }
    }
}

void Inspector::buildSceneSystemPanel()
{
    if (m_scenePanel) {
        m_scenePanel->deleteLater();
        m_scenePanel = nullptr;
    }
    m_systemFieldsContainer = nullptr;
    m_systemLayout = nullptr;

    m_scenePanel = new QWidget(m_systemContent);
    auto *outerLayout = new QVBoxLayout(m_scenePanel);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(4);

    auto *header = new QLabel(tr("场景系统"), m_scenePanel);
    header->setStyleSheet("font-weight: bold; font-size: 13px; color: #b0c4d8; padding: 4px 0;");
    outerLayout->addWidget(header);

    auto *sep = new QFrame(m_scenePanel);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #2a3540;");
    outerLayout->addWidget(sep);

    m_systemLayout = new QVBoxLayout();
    m_systemLayout->setSpacing(2);
    m_systemLayout->setContentsMargins(0, 0, 0, 0);

    // 每系统一行
    for (const auto& name : m_scene->getRegisteredSystemTypeNames()) {
        auto* sys = m_scene->getSystem(name);
        if (!sys) continue;

        auto *row = new QWidget(m_scenePanel);
        auto *hlay = new QHBoxLayout(row);
        hlay->setContentsMargins(2, 1, 2, 1);
        hlay->setSpacing(4);

        // 系统名（可点击选中展开字段编辑）
        auto *nameBtn = new QPushButton(QString::fromStdString(name), row);
        nameBtn->setFlat(true);
        nameBtn->setCursor(Qt::PointingHandCursor);
        nameBtn->setStyleSheet(
            "QPushButton { text-align: left; border: none; color: #b0c4d8; padding: 2px 4px; }"
            "QPushButton:hover { background: rgba(122,192,255,0.1); }");
        QString sysName = QString::fromStdString(name);
        connect(nameBtn, &QPushButton::clicked, this, [this, sysName] {
            m_selectedSystemName = (m_selectedSystemName == sysName) ? QString() : sysName;
            rebuildSystemPanel();
        });
        hlay->addWidget(nameBtn, 1);

        // 优先级 SpinBox
        auto *spin = new QSpinBox(row);
        spin->setRange(-1000, 1000);
        spin->setValue(sys->getPriority());
        spin->setFixedWidth(60);
        spin->setToolTip(tr("优先级（越小越先执行）"));
        std::string sysTypeName = name; // capture for lambda
        connect(spin, &QSpinBox::valueChanged, this, [this, sysTypeName](int v) {
            emit fieldEdited();  // undo begin
            setSystemPriority(sysTypeName, v);
            emit systemPriorityChanged();
        });
        hlay->addWidget(spin);

        // 移除按钮
        auto *removeBtn = new QPushButton(tr("✕"), row);
        removeBtn->setFixedWidth(24);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setStyleSheet(
            "QPushButton { border: none; color: #7a8a9a; }"
            "QPushButton:hover { color: #ff6b6b; }");
        connect(removeBtn, &QPushButton::clicked, this, [this, sysTypeName] {
            emit fieldEdited();  // undo begin
            removeSystemFromScene(sysTypeName);
            emit systemRemoved();
        });
        hlay->addWidget(removeBtn);

        // 选中高亮
        if (m_selectedSystemName == QString::fromStdString(name)) {
            row->setStyleSheet("background: rgba(122,192,255,0.12);");
        }

        m_systemLayout->addWidget(row);
    }
    outerLayout->addLayout(m_systemLayout);

    // 选中系统的字段编辑
    if (!m_selectedSystemName.isEmpty()) {
        auto *sys = m_scene->getSystem(m_selectedSystemName.toStdString());
        if (sys) {
            auto *fieldsContainer = new QWidget(m_scenePanel);
            m_systemFieldsContainer = fieldsContainer;
            auto *fieldsFormLayout = new QFormLayout(fieldsContainer);
            m_systemFieldsForm = fieldsFormLayout;
            fieldsFormLayout->setContentsMargins(8, 2, 0, 2);
            fieldsFormLayout->setSpacing(2);

            auto *fieldHeader = new QLabel(tr("系统属性"), fieldsContainer);
            fieldHeader->setStyleSheet("font-size: 11px; color: #7a8a9a; padding: 2px 0;");
            fieldsFormLayout->addRow(fieldHeader);

            const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*sys)));
            if (ti) {
                for (const auto& field : ti->fields) {
                    addSystemFieldRow(field, sys);
                }
            }
            outerLayout->addWidget(fieldsContainer);
        }
    }

    // 添加系统按钮
    auto *addRow = new QWidget(m_scenePanel);
    auto *addLayout = new QHBoxLayout(addRow);
    addLayout->setContentsMargins(0, 4, 0, 0);
    addLayout->addStretch(1);
    auto *addBtn = new QPushButton(tr("添加系统"), addRow);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { border: 1px dashed #3a4a5a; border-radius: 3px; color: #7a8a9a;"
        "              background: transparent; padding: 5px 12px; }"
        "QPushButton:hover { border-color: #7ac0ff; color: #d0e4f5; }");
    connect(addBtn, &QPushButton::clicked, this, &Inspector::showAddSystemMenu);
    addLayout->addWidget(addBtn);
    addLayout->addStretch(1);
    outerLayout->addWidget(addRow);

    outerLayout->addStretch(1);
    m_systemPageLayout->addWidget(m_scenePanel);
}

void Inspector::rebuildSystemPanel()
{
    if (!m_scene) return;
    if (m_scenePanel) {
        m_scenePanel->deleteLater();
        m_scenePanel = nullptr;
    }
    // 只清系统页回读（防 refresh 调用已删除控件的 lambda）；组件页 m_readbacks 不受影响
    m_systemReadbacks.clear();
    m_systemFieldsContainer = nullptr;
    m_systemLayout = nullptr;
    m_systemFieldsForm = nullptr;
    buildSceneSystemPanel();
    if (m_playMode) applyEditLock();
}

void Inspector::showAddSystemMenu()
{
    if (!m_scene) return;
    auto *menu = buildAddSystemMenu(this, m_scene,
        [this](const Shit::TypeInfo *type) {
            addSystemToScene(type);
        });
    menu->exec(QCursor::pos());
    delete menu;
}

void Inspector::addSystemToScene(const Shit::TypeInfo *type)
{
    if (!m_scene || !type) return;
    emit fieldEdited();  // undo begin
    m_scene->registerSystem(type->name);
    m_systemsSignature.clear();
    rebuildSystemPanel();
    emit systemAdded();
}

void Inspector::removeSystemFromScene(const std::string &typeName)
{
    if (!m_scene) return;
    m_scene->unregisterSystem(typeName);
    if (m_selectedSystemName == QString::fromStdString(typeName))
        m_selectedSystemName.clear();
    m_systemsSignature.clear();
    rebuildSystemPanel();
    // 注意：undo commit 由调用者（systemAdded/systemRemoved）负责
}

void Inspector::setSystemPriority(const std::string &typeName, int priority)
{
    if (!m_scene) return;
    m_scene->setSystemPriority(typeName, priority);
    m_systemsSignature.clear();
    // undo commit 由调用者（systemPriorityChanged 信号）负责
}

void Inspector::addSystemFieldRow(const Shit::FieldInfo &field, Shit::System *sys)
{
    // 复用 addFieldRow 的字段编辑逻辑，但用 System* 替代 Component*
    const bool readOnly = metaOf(field) && metaOf(field)->readOnly;
    const QString name = displayNameOf(field);
    if (readOnly) {
        QString val = QString("<%1>").arg(QString::fromStdString(field.typeName));
        // 尝试读取
        void *ptr = field.GetFieldPtr(sys);
        if (field.typeName == "float") val = QString::number(*reinterpret_cast<float*>(ptr));
        else if (field.typeName == "int") val = QString::number(*reinterpret_cast<int*>(ptr));
        else if (field.typeName == "bool") val = *reinterpret_cast<bool*>(ptr) ? tr("是") : tr("否");
        else if (field.typeName == "Vector2") {
            auto *v = reinterpret_cast<Shit::Vector2*>(ptr);
            val = QString("(%1, %2)").arg(v->x).arg(v->y);
        }
        else if (field.typeName == "std::string") val = QString::fromStdString(*reinterpret_cast<std::string*>(ptr));
        m_systemFieldsForm->addRow(name, new QLabel(val, m_content));
        return;
    }

    if (field.typeName == "float") {
        auto *box = new QDoubleSpinBox(m_content);
        const FieldMeta *m = metaOf(field);
        box->setRange(m && m->range.min != m->range.max ? m->range.min : -1e6,
                      m && m->range.min != m->range.max ? m->range.max : 1e6);
        box->setSingleStep(m && m->step > 0.0f ? m->step : 0.1f);
        box->setDecimals(3);
        box->setValue(*reinterpret_cast<float*>(field.GetFieldPtr(sys)));
        connect(box, &QDoubleSpinBox::valueChanged, this, [this, sys, field](double v) {
            emit fieldEdited();   // 先 begin（undo before 快照取改前状态）再写值
            *reinterpret_cast<float*>(field.GetFieldPtr(sys)) = static_cast<float>(v);
            sys->onFieldChanged(field.name);
        });
        connect(box, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_systemFieldsForm->addRow(name, box);
        m_systemReadbacks.push_back([box, sys, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<float*>(field.GetFieldPtr(sys)));
            box->blockSignals(false);
        });
    }
    else if (field.typeName == "int" && field.size == sizeof(int)) {
        auto *box = new QSpinBox(m_content);
        box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        box->setValue(*reinterpret_cast<int*>(field.GetFieldPtr(sys)));
        connect(box, &QSpinBox::valueChanged, this, [this, sys, field](int v) {
            emit fieldEdited();
            *reinterpret_cast<int*>(field.GetFieldPtr(sys)) = v;
            sys->onFieldChanged(field.name);
        });
        connect(box, &QSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_systemFieldsForm->addRow(name, box);
        m_systemReadbacks.push_back([box, sys, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<int*>(field.GetFieldPtr(sys)));
            box->blockSignals(false);
        });
    }
    else if (field.typeName == "bool") {
        auto *check = new QCheckBox(m_content);
        check->setChecked(*reinterpret_cast<bool*>(field.GetFieldPtr(sys)));
        connect(check, &QCheckBox::toggled, this, [this, sys, field](bool on) {
            emit fieldEdited();
            *reinterpret_cast<bool*>(field.GetFieldPtr(sys)) = on;
            sys->onFieldChanged(field.name);
            emit fieldCommitted();
        });
        m_systemFieldsForm->addRow(name, check);
        m_systemReadbacks.push_back([check, sys, field] {
            check->blockSignals(true);
            check->setChecked(*reinterpret_cast<bool*>(field.GetFieldPtr(sys)));
            check->blockSignals(false);
        });
    }
    else if (field.typeName == "Vector2") {
        auto *p = reinterpret_cast<Shit::Vector2*>(field.GetFieldPtr(sys));
        auto *xBox = makeSpin(p->x);
        auto *yBox = makeSpin(p->y);
        connect(xBox, &QDoubleSpinBox::valueChanged, this, [this, sys, field](double v) {
            emit fieldEdited();
            reinterpret_cast<Shit::Vector2*>(field.GetFieldPtr(sys))->x = static_cast<float>(v);
            sys->onFieldChanged(field.name);
        });
        connect(yBox, &QDoubleSpinBox::valueChanged, this, [this, sys, field](double v) {
            emit fieldEdited();
            reinterpret_cast<Shit::Vector2*>(field.GetFieldPtr(sys))->y = static_cast<float>(v);
            sys->onFieldChanged(field.name);
        });
        connect(xBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        connect(yBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        auto *row = new QWidget(m_content);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(xBox);
        layout->addWidget(yBox);
        m_systemFieldsForm->addRow(name, row);
        m_systemReadbacks.push_back([xBox, yBox, sys, field] {
            auto *p = reinterpret_cast<Shit::Vector2*>(field.GetFieldPtr(sys));
            xBox->blockSignals(true);
            yBox->blockSignals(true);
            xBox->setValue(p->x);
            yBox->setValue(p->y);
            xBox->blockSignals(false);
            yBox->blockSignals(false);
        });
    }
    else if (field.typeName == "std::string") {
        auto *p = reinterpret_cast<std::string*>(field.GetFieldPtr(sys));
        auto *edit = new QLineEdit(QString::fromStdString(*p), m_content);
        connect(edit, &QLineEdit::textChanged, this, [this, sys, field](const QString &text) {
            emit fieldEdited();
            *reinterpret_cast<std::string*>(field.GetFieldPtr(sys)) = text.toStdString();
            sys->onFieldChanged(field.name);
        });
        connect(edit, &QLineEdit::editingFinished, this, [this] { emit fieldCommitted(); });
        m_systemFieldsForm->addRow(name, edit);
        m_systemReadbacks.push_back([edit, sys, field] {
            edit->blockSignals(true);
            edit->setText(QString::fromStdString(*reinterpret_cast<std::string*>(field.GetFieldPtr(sys))));
            edit->blockSignals(false);
        });
    }
    else {
        // 枚举或未知类型：只读展示
        const Shit::TypeInfo *enumType = Shit::TypeRegistry::Get(field.typeName);
        if (enumType && !enumType->enumValues.empty()) {
            auto *combo = new QComboBox(m_content);
            const int cur = *reinterpret_cast<int*>(field.GetFieldPtr(sys));
            int sel = 0;
            for (size_t i = 0; i < enumType->enumValues.size(); ++i) {
                combo->addItem(QString::fromStdString(enumType->enumValues[i].name),
                               static_cast<int>(enumType->enumValues[i].value));
                if (enumType->enumValues[i].value == cur) sel = static_cast<int>(i);
            }
            combo->setCurrentIndex(sel);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, sys, field, combo]() {
                emit fieldEdited();
                *reinterpret_cast<int*>(field.GetFieldPtr(sys)) = combo->currentData().toInt();
                sys->onFieldChanged(field.name);
                emit fieldCommitted();
            });
            m_systemFieldsForm->addRow(name, combo);
            m_systemReadbacks.push_back([combo, sys, field] {
                const int v = *reinterpret_cast<int*>(field.GetFieldPtr(sys));
                const int idx = combo->findData(v);
                combo->blockSignals(true);
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
                combo->blockSignals(false);
            });
        } else {
            m_systemFieldsForm->addRow(name, new QLabel(QString("<%1>").arg(typeNameOf(field)), m_content));
        }
    }
}

void Inspector::addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj)
{
    const bool readOnly = metaOf(field) && metaOf(field)->readOnly;
    const QString name = displayNameOf(field);

    // 组件引用字段（ComponentRef<T>）→ 拖拽引用控件
    // 防御：ComponentRef<T> 恒为 8 字节；size 不符（扫描器误解析等）时回退普通只读展示
    if (field.isReference() && field.size == sizeof(uint64_t)) {
        if (readOnly) {
            auto *w = new RefFieldWidget(obj, field, [] {}, m_content);
            w->setReadOnly(true);
            m_form->addRow(name, w);
            m_readbacks.push_back([w] { w->refresh(); });
            return;
        }
        auto *widget = new RefFieldWidget(obj, field, [this] {
            emit fieldEdited();
            emit fieldCommitted();
        }, m_content);
        m_form->addRow(name, widget);
        m_readbacks.push_back([widget] { widget->refresh(); });
        return;
    }

    if (readOnly) {
        m_form->addRow(name, new QLabel(fieldToString(field, obj), m_content));
        return;
    }

    if (typeNameOf(field) == "float") {
        auto *box = new QDoubleSpinBox(m_content);
        const FieldMeta *m = metaOf(field);
        if (m && m->range.min != m->range.max)
            box->setRange(m->range.min, m->range.max);
        else
            box->setRange(-1e6, 1e6);
        box->setSingleStep(m && m->step > 0.0f ? m->step : 0.1f);
        box->setDecimals(3);
        box->setValue(*reinterpret_cast<float *>(field.GetFieldPtr(obj)));
        connect(box, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            emit fieldEdited();   // 先 begin（undo before 快照取改前状态）再写值
            *reinterpret_cast<float *>(field.GetFieldPtr(obj)) = static_cast<float>(v);
            obj->onFieldChanged(field.name);
        });
        connect(box, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, obj, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<float *>(field.GetFieldPtr(obj)));
            box->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "int" && field.size == sizeof(int)) {
        // size 校验：libclang 会把解析失败的模板字段（std::vector 等）退化为
        // "int"，此时 size 为真实对象大小（如 24），按 4 字节读写会损坏内存 → 只读展示
        auto *box = new QSpinBox(m_content);
        box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        box->setValue(*reinterpret_cast<int *>(field.GetFieldPtr(obj)));
        connect(box, &QSpinBox::valueChanged, this, [this, obj, field](int v) {
            emit fieldEdited();
            *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = v;
            obj->onFieldChanged(field.name);
        });
        connect(box, &QSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, obj, field] {
            box->blockSignals(true);
            box->setValue(*reinterpret_cast<int *>(field.GetFieldPtr(obj)));
            box->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "bool") {
        auto *check = new QCheckBox(m_content);
        check->setChecked(*reinterpret_cast<bool *>(field.GetFieldPtr(obj)));
        connect(check, &QCheckBox::toggled, this, [this, obj, field](bool on) {
            emit fieldEdited();
            *reinterpret_cast<bool *>(field.GetFieldPtr(obj)) = on;
            obj->onFieldChanged(field.name);
            emit fieldCommitted();
        });
        m_form->addRow(name, check);
        m_readbacks.push_back([check, obj, field] {
            check->blockSignals(true);
            check->setChecked(*reinterpret_cast<bool *>(field.GetFieldPtr(obj)));
            check->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "Vector2") {
        auto *p = reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj));
        auto *xBox = makeSpin(p->x);
        auto *yBox = makeSpin(p->y);
        connect(xBox, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            emit fieldEdited();
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->x = static_cast<float>(v);
            obj->onFieldChanged(field.name);
        });
        connect(yBox, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            emit fieldEdited();
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->y = static_cast<float>(v);
            obj->onFieldChanged(field.name);
        });
        connect(xBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        connect(yBox, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        auto *row = new QWidget(m_content);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(xBox);
        layout->addWidget(yBox);
        m_form->addRow(name, row);
        m_readbacks.push_back([xBox, yBox, obj, field] {
            auto *p = reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj));
            xBox->blockSignals(true);
            yBox->blockSignals(true);
            xBox->setValue(p->x);
            yBox->setValue(p->y);
            xBox->blockSignals(false);
            yBox->blockSignals(false);
        });
    }
    else if (typeNameOf(field) == "std::string") {
        auto *p = reinterpret_cast<std::string *>(field.GetFieldPtr(obj));
        // 路径类字段（字段名语义命中）→ 统一路径控件：拖拽填充 + 浏览选择 + 手输解析，
        // 提交值经 AssetPaths 规范化（项目内相对存储），一次一提交接撤销栈
        if (const PathFieldSpec spec = pathSpecForFieldName(field.name); spec.isPath && !readOnly) {
            auto *pathField = new PathFieldWidget(spec, m_content);
            pathField->setPath(QString::fromStdString(*p));
            connect(pathField, &PathFieldWidget::pathCommitted, this,
                    [this, obj, field, p](const QString &stored) {
                        emit fieldEdited();               // undo begin（须在修改前）
                        *p = stored.toStdString();
                        obj->onFieldChanged(field.name);
                        emit fieldCommitted();             // undo commit
                    });
            m_form->addRow(name, pathField);
            m_readbacks.push_back([pathField, p] {
                pathField->setPath(QString::fromStdString(*p));
            });
        }
        else {
            auto *edit = new QLineEdit(QString::fromStdString(*p), m_content);
            connect(edit, &QLineEdit::textChanged, this, [this, obj, field](const QString &text) {
                emit fieldEdited();
                *reinterpret_cast<std::string *>(field.GetFieldPtr(obj)) = text.toStdString();
                obj->onFieldChanged(field.name);
            });
            connect(edit, &QLineEdit::editingFinished, this, [this] { emit fieldCommitted(); });
            m_form->addRow(name, edit);
            m_readbacks.push_back([edit, obj, field] {
                edit->blockSignals(true);
                edit->setText(QString::fromStdString(*reinterpret_cast<std::string *>(field.GetFieldPtr(obj))));
                edit->blockSignals(false);
            });
        }
    }
    else {
        // 枚举或未知类型：优先尝试枚举下拉，否则只读展示
        const Shit::TypeInfo *enumType = Shit::TypeRegistry::Get(typeNameOf(field).toStdString());
        if (enumType && !enumType->enumValues.empty()) {
            auto *combo = new QComboBox(m_content);
            const int cur = *reinterpret_cast<int *>(field.GetFieldPtr(obj));
            int sel = 0;
            for (size_t i = 0; i < enumType->enumValues.size(); ++i) {
                combo->addItem(QString::fromStdString(enumType->enumValues[i].name),
                               static_cast<int>(enumType->enumValues[i].value));
                if (enumType->enumValues[i].value == cur) sel = static_cast<int>(i);
            }
            combo->setCurrentIndex(sel);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, obj, field, combo]() {
                emit fieldEdited();
                *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = combo->currentData().toInt();
                obj->onFieldChanged(field.name);
                emit fieldCommitted();
            });
            m_form->addRow(name, combo);
            m_readbacks.push_back([combo, obj, field] {
                const int v = *reinterpret_cast<int *>(field.GetFieldPtr(obj));
                const int idx = combo->findData(v);
                combo->blockSignals(true);
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
                combo->blockSignals(false);
            });
        } else {
            m_form->addRow(name, new QLabel(fieldToString(field, obj), m_content));
        }
    }
}

QString Inspector::fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const
{
    void *ptr = field.GetFieldPtr(obj);
    if (typeNameOf(field) == "float")
        return QString::number(*reinterpret_cast<float *>(ptr), 'g', 4);
    if (typeNameOf(field) == "int")
        return QString::number(*reinterpret_cast<int *>(ptr));
    if (typeNameOf(field) == "bool")
        return *reinterpret_cast<bool *>(ptr) ? tr("是") : tr("否");
    if (typeNameOf(field) == "Vector2") {
        auto *v = reinterpret_cast<Shit::Vector2 *>(ptr);
        return QString("(%1, %2)").arg(v->x).arg(v->y);
    }
    if (typeNameOf(field) == "std::string")
        return QString::fromStdString(*reinterpret_cast<std::string *>(ptr));
    return QString("<%1>").arg(typeNameOf(field));
}

// ═══════════════════════════════════════════════════════════════
// 属性渲染（getter/setter 对）
// ═══════════════════════════════════════════════════════════════

void Inspector::addPropertyRow(const Shit::PropertyInfo &prop, Shit::Component *obj)
{
    // 获取属性元数据（取第一个非空 displayName）
    const FieldMeta *m = nullptr;
    if (!prop.meta.empty()) m = &prop.meta[0];

    const QString name = m && !m->displayName.empty()
        ? QString::fromStdString(m->displayName)
        : QString::fromStdString(prop.name);

    const bool readOnly = m && m->readOnly;
    const QString typeName = QString::fromStdString(prop.typeName);

    // 通过 getter 读取当前值
    void *value = prop.getter(obj);

    if (readOnly || prop.setter == nullptr) {
        // 只读：显示为标签（每帧回读刷新，运行态实时值可见）
        auto *label = new QLabel(m_content);
        auto formatValue = [typeName](void *v) -> QString {
            if (!v) return QString();
            if (typeName == "float")
                return QString::number(*static_cast<float *>(v), 'g', 4);
            if (typeName == "int")
                return QString::number(*static_cast<int *>(v));
            if (typeName == "bool")
                return *static_cast<bool *>(v) ? QObject::tr("是") : QObject::tr("否");
            return QStringLiteral("<%1>").arg(typeName);
        };
        label->setText(formatValue(value));
        m_form->addRow(name, label);
        m_readbacks.push_back([label, prop, obj, formatValue] {
            label->setText(formatValue(prop.getter(obj)));
        });
        return;
    }

    if (typeName == "float") {
        auto *box = new QDoubleSpinBox(m_content);
        if (m && m->range.min != m->range.max)
            box->setRange(m->range.min, m->range.max);
        else
            box->setRange(-1e6, 1e6);
        box->setSingleStep(m && m->step > 0.0f ? m->step : 0.1f);
        box->setDecimals(3);
        box->setValue(*static_cast<float *>(value));
        connect(box, &QDoubleSpinBox::valueChanged, this, [this, obj, prop](double v) {
            // 先 begin（undo before 快照取改前状态）再写值——顺序颠倒会使单步编辑不入撤销栈
            emit fieldEdited();
            float fv = static_cast<float>(v);
            prop.setter(obj, &fv);
        });
        connect(box, &QDoubleSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, prop, obj] {
            box->blockSignals(true);
            void *v = prop.getter(obj);
            box->setValue(*static_cast<float *>(v));
            box->blockSignals(false);
        });
    }
    else if (typeName == "int") {
        auto *box = new QSpinBox(m_content);
        if (m && m->range.min != m->range.max)
            box->setRange(static_cast<int>(m->range.min), static_cast<int>(m->range.max));
        else
            box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        box->setValue(*static_cast<int *>(value));
        connect(box, &QSpinBox::valueChanged, this, [this, obj, prop](int v) {
            emit fieldEdited();
            prop.setter(obj, &v);
        });
        connect(box, &QSpinBox::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, box);
        m_readbacks.push_back([box, prop, obj] {
            box->blockSignals(true);
            void *v = prop.getter(obj);
            box->setValue(*static_cast<int *>(v));
            box->blockSignals(false);
        });
    }
    else if (typeName == "bool") {
        auto *check = new QCheckBox(m_content);
        check->setChecked(*static_cast<bool *>(value));
        connect(check, &QCheckBox::toggled, this, [this, obj, prop](bool on) {
            emit fieldEdited();
            prop.setter(obj, &on);
            emit fieldCommitted();
        });
        m_form->addRow(name, check);
        m_readbacks.push_back([check, prop, obj] {
            check->blockSignals(true);
            void *v = prop.getter(obj);
            check->setChecked(*static_cast<bool *>(v));
            check->blockSignals(false);
        });
    }
    else if (typeName == "std::string") {
        auto *str = static_cast<std::string *>(value);
        auto *edit = new QLineEdit(QString::fromStdString(*str), m_content);
        connect(edit, &QLineEdit::textChanged, this, [this, obj, prop](const QString &text) {
            emit fieldEdited();
            std::string s = text.toStdString();
            prop.setter(obj, &s);
        });
        connect(edit, &QLineEdit::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, edit);
        m_readbacks.push_back([edit, prop, obj] {
            edit->blockSignals(true);
            void *v = prop.getter(obj);
            edit->setText(QString::fromStdString(*static_cast<std::string *>(v)));
            edit->blockSignals(false);
        });
    }
    else {
        // 未知类型：只读展示
        m_form->addRow(name, new QLabel(QString("<%1>").arg(typeName), m_content));
    }
}

void Inspector::showAddComponentMenu()
{
    if (!m_object) return;
    auto *menu = buildAddComponentMenu(this, m_object,
        [this](const Shit::TypeInfo *ti) { addComponentToObject(ti); });
    menu->exec(QCursor::pos());
    delete menu;
}

void Inspector::addComponentToObject(const Shit::TypeInfo *type)
{
    if (!type || !m_object) return;
    void *raw = type->Create();   // 反射工厂堆分配（菜单已过滤无工厂/抽象类型）
    if (!raw) return;
    auto *comp = static_cast<Shit::Component *>(raw);
    emit fieldEdited();           // undo begin + 会话 dirty（须在修改前；mainwindow 已连）
    m_object->addComponentInstance(comp);
    setGameObject(m_object);      // 重建表单显示新组件
    emit componentAdded();        // undo commit（标签"添加组件"）
}

void Inspector::removeComponentFromObject(Shit::Component *component)
{
    if (!component || !m_object) return;
    const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*component)));
    if (!ti) return;   // 无反射元数据（不可达：组件头只对反射类型渲染）

    // 基础设施拒删（与场景树"scene_camera 拒删"同语义）：
    // TransformComponent 每个对象必有（渲染/物理/UI 依赖），Unity 同款不可移除；
    // scene_camera 的相机组件是场景视图编辑视点，移除后编辑 pass 失能。
    const QString typeName = normalizeTypeName(ti->name);
    if (typeName == "TransformComponent") {
        emit componentRemoveBlocked(tr("TransformComponent 是对象的基础组件，不能移除"));
        return;
    }
    if (m_object->getName() == "scene_camera" && typeName == "CameraComponent") {
        emit componentRemoveBlocked(tr("scene_camera 的相机组件是编辑器基础设施，不能移除"));
        return;
    }

    emit fieldEdited();   // undo begin + dirty（须在修改前）
    m_object->removeComponent(std::type_index(typeid(*component)));
    setGameObject(m_object);   // 重建表单；引用该组件的字段随即显示 None（uuid 索引已清）
    emit componentRemoved();   // undo commit（标签"移除组件"）
}