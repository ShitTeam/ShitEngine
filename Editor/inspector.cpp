#include "inspector.h"

#include "componentmenu.h"
#include "dnd.h"
#include "animatoreditorwidget.h"
#include "animatorwidget.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Component/AnimationComponent.h>
#include <ShitEngine/Animation/Animator.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QDoubleSpinBox>
#include <QDrag>
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
// P20: 组件引用字段（ComponentRef<T>）——拖拽赋引用
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
        for (const auto &[uuid, typeName] : items) {
            Q_UNUSED(typeName);   // 以场景实况为准（类型校验走反射基类链）
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
    , m_scroll(new QScrollArea(this))
    , m_content(new QWidget)
    , m_form(new QFormLayout(m_content))
{
    m_scroll->setWidget(m_content);
    m_scroll->setWidgetResizable(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scroll);

    m_form->setContentsMargins(8, 8, 8, 8);
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    clear();
}

void Inspector::clear()
{
    m_readbacks.clear();
    m_object = nullptr;
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    auto *placeholder = new QLabel(tr("未选中对象\n\n选中场景中的对象后，\n在此编辑其组件属性。"), m_content);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    m_form->addRow(placeholder);
}

void Inspector::refresh()
{
    for (auto &readback : m_readbacks)
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

    // Unity 风格：顶部对象名编辑行（提交走撤销"重命名"；与树内 F2 等价，两处同步）
    auto *nameEdit = new QLineEdit(QString::fromStdString(m_object->getName()), m_content);
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
    m_form->addRow(nameEdit);

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
        // P20: 组件头可拖拽（携带自身 uuid + 类型名，供引用字段拖入）
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

        // P28：AnimationComponent 渲染专用剪辑编辑器（跳过反射字段：m_clipsData 只读载体）
        if (normalizeTypeName(typeInfo->name) == "AnimationComponent") {
            auto *animator = new AnimatorEditorWidget(static_cast<Shit::AnimationComponent *>(component), m_content);
            // 包装成整行（不占 label 列）
            auto *animRow = new QWidget(m_content);
            auto *animLayout = new QVBoxLayout(animRow);
            animLayout->setContentsMargins(0, 0, 0, 0);
            animLayout->addWidget(animator);
            m_form->addRow(animRow);
            m_readbacks.push_back([animator] { animator->refresh(); });
            connect(animator, &AnimatorEditorWidget::changed, this, [this] {
                emit fieldEdited();      // undo begin + dirty
                emit fieldCommitted();   // undo commit（剪辑编辑为一次性提交）
            });
        }
        // P28：Animator 状态机渲染专用编辑器（跳过反射字段：m_animatorData 只读载体）
        else if (normalizeTypeName(typeInfo->name) == "Animator") {
            auto *animator = new AnimatorWidget(static_cast<Shit::Animator *>(component), m_content);
            auto *animRow = new QWidget(m_content);
            auto *animLayout = new QVBoxLayout(animRow);
            animLayout->setContentsMargins(0, 0, 0, 0);
            animLayout->addWidget(animator);
            m_form->addRow(animRow);
            m_readbacks.push_back([animator] { animator->refresh(); });
            connect(animator, &AnimatorWidget::changed, this, [this] {
                emit fieldEdited();
                emit fieldCommitted();
            });
        }
        else {
            for (const Shit::FieldInfo &field : typeInfo->fields) {
                addFieldRow(field, component);
                ++m_fieldCount;
            }
        }
    });

    emit buildInfo(m_componentCount, m_fieldCount);

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
    // 统一递归禁用表单（含字段控件/移除按钮/Add Component/对象名栏/引用控件）；
    // 禁用控件仍可被回读刷新 setText/setValue（blockSignals 已防回环），运行时值实时可见
    if (m_content) m_content->setEnabled(!m_playMode);
}

void Inspector::addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj)
{
    const bool readOnly = metaOf(field) && metaOf(field)->readOnly;
    const QString name = displayNameOf(field);

    // P20: 组件引用字段（ComponentRef<T>）→ 拖拽引用控件
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
            *reinterpret_cast<float *>(field.GetFieldPtr(obj)) = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
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
            *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = v;
            obj->onFieldChanged(field.name);
            emit fieldEdited();
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
            *reinterpret_cast<bool *>(field.GetFieldPtr(obj)) = on;
            obj->onFieldChanged(field.name);
            emit fieldEdited();
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
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->x = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(yBox, &QDoubleSpinBox::valueChanged, this, [this, obj, field](double v) {
            reinterpret_cast<Shit::Vector2 *>(field.GetFieldPtr(obj))->y = static_cast<float>(v);
            obj->onFieldChanged(field.name);
            emit fieldEdited();
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
        auto *edit = new QLineEdit(QString::fromStdString(*p), m_content);
        connect(edit, &QLineEdit::textChanged, this, [this, obj, field](const QString &text) {
            *reinterpret_cast<std::string *>(field.GetFieldPtr(obj)) = text.toStdString();
            obj->onFieldChanged(field.name);
            emit fieldEdited();
        });
        connect(edit, &QLineEdit::editingFinished, this, [this] { emit fieldCommitted(); });
        m_form->addRow(name, edit);
        m_readbacks.push_back([edit, obj, field] {
            edit->blockSignals(true);
            edit->setText(QString::fromStdString(*reinterpret_cast<std::string *>(field.GetFieldPtr(obj))));
            edit->blockSignals(false);
        });
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
                *reinterpret_cast<int *>(field.GetFieldPtr(obj)) = combo->currentData().toInt();
                obj->onFieldChanged(field.name);
                emit fieldEdited();
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