#include "scenetree.h"

#include "componentmenu.h"
#include "scenetreemodel.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Component/TransformComponent.h>
#include <ShitEngine/Component/CameraComponent.h>
#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/UI/UICanvas.h>
#include <ShitEngine/UI/UIText.h>

#include <QAbstractItemView>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

#include <functional>

SceneTree::SceneTree(QWidget *parent)
    : QWidget(parent)
    , m_view(new QTreeView(this))
    , m_model(new SceneTreeModel(this))
    , m_filterEdit(new QLineEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // P34：名称过滤框（Ctrl+F 聚焦；输入即过滤，清空恢复全显）
    m_filterEdit->setPlaceholderText(tr("过滤对象名称…（Ctrl+F）"));
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #3a4a5a; border-radius: 3px;"
        "             background: #1c2430; color: #e0e8f0; padding: 3px 6px; }"
        "QLineEdit:focus { border-color: #7ac0ff; }");
    layout->addWidget(m_filterEdit);

    layout->addWidget(m_view);

    m_view->setModel(m_model);
    m_view->setHeaderHidden(true);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);  // P25a：多选（Ctrl/Shift）
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    // P11：双击/F2 重命名 + 拖拽改层级
    m_view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_view->setDragEnabled(true);
    m_view->setAcceptDrops(true);
    m_view->setDropIndicatorShown(true);
    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    m_view->setDefaultDropAction(Qt::MoveAction);

    connect(m_filterEdit, &QLineEdit::textChanged, this, [this] { applyFilter(); });

    // P34：Ctrl+F 聚焦过滤框（拖拽/重命名编辑中不劫持）
    auto *filterAction = new QAction(this);
    filterAction->setShortcut(QKeySequence::Find);
    filterAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(filterAction);
    connect(filterAction, &QAction::triggered, this, [this] {
        m_filterEdit->setFocus();
        m_filterEdit->selectAll();
    });

    // 选中 → objectSelected，供属性检查器联动
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) {
                emit objectSelected(m_model->gameObjectAt(current));
            });

    // P11 撤销接线：按下（含进入编辑/发起拖拽）→ begin；模型编辑完成（重命名/改层级）→ 提交
    connect(m_view, &QTreeView::pressed, this, [this] { emit sceneActionStarted(); });
    connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &) {
        emit sceneActionStarted();
    });
    connect(m_model, &SceneTreeModel::dataEdited, this, [this] { emit sceneEdited(); });

    // P13：Del 删除选中对象（树获焦时生效；重命名编辑中豁免）
    auto *deleteAction = new QAction(tr("删除对象"), this);
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(deleteAction);
    connect(deleteAction, &QAction::triggered, this, &SceneTree::deleteObject);
}

void SceneTree::setScene(Shit::Scene *scene, bool autoSelect)
{
    m_scene = scene;
    m_model->setScene(scene);
    m_view->expandAll();
    applyFilter();   // P34：重建树后保持当前过滤状态

    if (!autoSelect) return;

    // 自动选中第一项，触发 objectSelected → 检查器
    if (m_model->rowCount({}) > 0) {
        const QModelIndex first = m_model->index(0, 0, {});
        m_view->selectionModel()->setCurrentIndex(first,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

Shit::GameObject *SceneTree::selectedObject() const
{
    return m_model->gameObjectAt(m_view->selectionModel()->currentIndex());
}

QList<Shit::GameObject *> SceneTree::selectedObjects() const
{
    QList<Shit::GameObject *> objects;
    const auto rows = m_view->selectionModel()->selectedRows();
    for (const QModelIndex &idx : rows) {
        if (Shit::GameObject *go = m_model->gameObjectAt(idx))
            objects.append(go);
    }
    return objects;
}

void SceneTree::selectObject(Shit::GameObject *object)
{
    const QModelIndex idx = m_model->indexOf(object);
    if (idx.isValid()) {
        m_view->selectionModel()->setCurrentIndex(idx,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
        m_view->scrollTo(idx);
    }
}

void SceneTree::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex idx = m_view->indexAt(event->pos());
    Shit::GameObject *target = idx.isValid() ? m_model->gameObjectAt(idx) : nullptr;
    if (target)
        m_view->setCurrentIndex(idx);

    auto *menu = new QMenu(this);
    auto *newMenu = menu->addMenu(tr("新建"));
    newMenu->addAction(tr("空对象"), this, [this] { createObject(); });
    // 模板创建（Unity 式快捷搭建）：同名自动去重后缀 (1) (2)…
    newMenu->addAction(tr("精灵"), this, [this] { createObjectOfKind(CreateKind::Sprite); });
    newMenu->addAction(tr("相机"), this, [this] { createObjectOfKind(CreateKind::Camera); });
    newMenu->addAction(tr("UI Canvas"), this, [this] { createObjectOfKind(CreateKind::Canvas); });
    newMenu->addAction(tr("文本"), this, [this] { createObjectOfKind(CreateKind::Text); });

    if (target) {
        const int multiCount = m_view->selectionModel()->selectedRows().size();
        menu->addSeparator();
        menu->addMenu(buildAddComponentMenu(this, target,
            [this, target](const Shit::TypeInfo *ti) { addComponent(ti, target); }));
        // P25c：把对象（含子树）存为 .prefab 预置资产（多选时保存当前项）
        QAction *prefabAction = nullptr;
        if (multiCount <= 1) {
            prefabAction = menu->addAction(tr("存为预置…"));
            connect(prefabAction, &QAction::triggered, this, [this, target] {
                emit prefabSaveRequested(target);
            });
        }
        auto *delAction = menu->addAction(multiCount > 1
            ? tr("删除对象 (%1)").arg(multiCount) : tr("删除对象"));
        connect(delAction, &QAction::triggered, this, &SceneTree::deleteObject);
    }

    menu->exec(event->globalPos());
    delete menu;
}

void SceneTree::applyFilter()
{
    const QString text = m_filterEdit->text().trimmed();

    // 递归判定：节点自身名称匹配 或 任一后代匹配 → 显示（匹配节点保留整条祖先链）
    std::function<bool(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
        bool anyShown = false;
        for (int r = 0; r < m_model->rowCount(parent); ++r) {
            const QModelIndex idx = m_model->index(r, 0, parent);
            const bool self = text.isEmpty()
                || m_model->data(idx, Qt::DisplayRole).toString().contains(text, Qt::CaseInsensitive);
            const bool childShown = walk(idx);
            const bool show = self || childShown;
            m_view->setRowHidden(r, parent, !show);
            anyShown = anyShown || show;
        }
        return anyShown;
    };
    walk({});
}

void SceneTree::createObject()
{
    createObjectOfKind(CreateKind::Empty);
}

void SceneTree::createObjectOfKind(CreateKind kind)
{
    if (!m_scene) return;

    // 场景内唯一名（已有同名则 " (1)"、" (2)"…，与主窗口复制粘贴一致）
    const auto &gos = m_scene->getGameObjects();
    auto taken = [&](const std::string &name) {
        for (const auto &go : gos)
            if (go->getName() == name) return true;
        return false;
    };
    auto uniqueName = [&](const char *base) {
        std::string name = base;
        if (!taken(name)) return name;
        for (int i = 1; ; ++i) {
            const std::string candidate = std::string(base) + " (" + std::to_string(i) + ")";
            if (!taken(candidate)) return candidate;
        }
    };

    emit sceneActionStarted();   // 撤销 begin（须在修改前）
    auto *go = m_scene->createGameObject(uniqueName("New Object"));
    go->addComponent<Shit::TransformComponent>();

    switch (kind) {
    case CreateKind::Sprite:
        go->setName(uniqueName("New Sprite"));
        go->addComponent<Shit::SpriteRenderer>();
        break;
    case CreateKind::Camera:
        go->setName(uniqueName("New Camera"));
        go->addComponent<Shit::CameraComponent>();
        break;
    case CreateKind::Canvas:
        go->setName(uniqueName("New Canvas"));
        go->addComponent<Shit::UICanvas>();
        break;
    case CreateKind::Text:
        // 文本渲染依赖 Canvas 父：优先挂到场景已有 Canvas，无则顺带创建一个（Unity 同款自动适配 UI 层级）
        go->setName(uniqueName("New Text"));
        go->addComponent<Shit::UIText>();
        {
            Shit::GameObject *canvas = nullptr;
            for (const auto &candidate : gos)
                if (candidate->getComponent<Shit::UICanvas>()) { canvas = candidate.get(); break; }
            if (!canvas) {
                canvas = m_scene->createGameObject("Canvas");
                canvas->addComponent<Shit::TransformComponent>();
                canvas->addComponent<Shit::UICanvas>();
            }
            go->setParent(canvas);
        }
        break;
    case CreateKind::Empty:
    default:
        break;   // 空对象：保持默认名与仅 Transform
    }

    m_model->setScene(m_scene);
    selectObject(go);
    emit sceneEdited();
}

void SceneTree::deleteObject()
{
    if (!m_scene) return;

    // P25a：批量删除（快照收集 → 逐个删，迭代中不直接删容器元素）
    const auto objects = selectedObjects();
    if (objects.isEmpty()) return;

    // scene_camera 是场景视图基础设施（不入库、树中隐藏）：删除后编辑视点失能，
    // 保存亦无法恢复。游戏相机不定名（Unity 语义）——场景中任意相机都可删除，
    // 运行视图会自动改选场景中其余相机或兜底编辑器相机。
    // 混选时只过滤基础设施，其余照删（并红字提示被跳过项）。
    QList<Shit::GameObject *> victims;
    for (Shit::GameObject *go : objects) {
        if (go->getName() == "scene_camera") {
            emit sceneDeleteBlocked(tr("scene_camera 是编辑器相机基础设施，不能删除"));
            continue;
        }
        victims.append(go);
    }
    if (victims.isEmpty()) return;

    emit sceneActionStarted();   // 撤销 begin（须在修改前）
    for (Shit::GameObject *go : victims)
        m_scene->removeGameObject(go);
    // 用 setScene 而非 m_model->setScene：自动重新选中第一项 → 触发 objectSelected，
    // 检查器/视口及时换绑新对象（否则其持有被删对象指针，下一帧渲染即悬垂崩溃）
    setScene(m_scene);
    emit sceneEdited();
}

void SceneTree::addComponent(const Shit::TypeInfo *type, Shit::GameObject *target)
{
    if (!type || !target) return;
    emit sceneActionStarted();   // 撤销 begin（须在修改前）
    void *raw = type->Create();   // factory 堆分配（非抽象）
    if (!raw) return;
    auto *comp = static_cast<Shit::Component *>(raw);
    target->addComponentInstance(comp);
    emit objectSelected(target);   // 刷新检查器显示新组件
    emit sceneEdited();
}
