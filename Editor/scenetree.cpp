#include "scenetree.h"

#include "scenetreemodel.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include <QAbstractItemView>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

namespace {

/// 判断反射类型是否为 Component 派生（沿基类链查找 "Component"）
bool isComponentDerived(const Shit::TypeInfo *ti)
{
    for (const Shit::TypeInfo *b = ti; b; b = b->baseType)
        if (b->name == "Component") return true;
    return false;
}

} // namespace

SceneTree::SceneTree(QWidget *parent)
    : QWidget(parent)
    , m_view(new QTreeView(this))
    , m_model(new SceneTreeModel(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    m_view->setModel(m_model);
    m_view->setHeaderHidden(true);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 选中 → objectSelected，供属性检查器联动
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) {
                emit objectSelected(m_model->gameObjectAt(current));
            });
}

void SceneTree::setScene(Shit::Scene *scene)
{
    m_scene = scene;
    m_model->setScene(scene);
    m_view->expandAll();

    // 自动选中第一项，触发 objectSelected → 检查器
    if (m_model->rowCount({}) > 0) {
        const QModelIndex first = m_model->index(0, 0, {});
        m_view->selectionModel()->setCurrentIndex(first,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
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
    auto *newAction = menu->addAction(tr("新建对象"));
    connect(newAction, &QAction::triggered, this, &SceneTree::createObject);

    if (target) {
        menu->addSeparator();
        menu->addMenu(buildAddComponentMenu(target));
        auto *delAction = menu->addAction(tr("删除对象"));
        connect(delAction, &QAction::triggered, this, &SceneTree::deleteObject);
    }

    menu->exec(event->globalPos());
    delete menu;
}

void SceneTree::createObject()
{
    if (!m_scene) return;
    auto *go = m_scene->createGameObject("New Object");
    go->addComponent<Shit::TransformComponent>();
    m_model->setScene(m_scene);   // 刷新层级
    selectObject(go);
}

void SceneTree::deleteObject()
{
    if (!m_scene) return;
    const QModelIndex idx = m_view->selectionModel()->currentIndex();
    Shit::GameObject *go = m_model->gameObjectAt(idx);
    if (go) {
        m_scene->removeGameObject(go);
        m_model->setScene(m_scene);   // 刷新层级（对象在帧末销毁）
    }
}

QMenu *SceneTree::buildAddComponentMenu(Shit::GameObject *target)
{
    auto *menu = new QMenu(tr("添加组件"), this);

    std::vector<const Shit::TypeInfo *> comps;
    Shit::TypeRegistry::ForEach([&](const Shit::TypeInfo &ti) {
        if (isComponentDerived(&ti) && ti.factory)
            comps.push_back(&ti);
    });
    std::sort(comps.begin(), comps.end(),
              [](const Shit::TypeInfo *a, const Shit::TypeInfo *b) { return a->name < b->name; });

    for (const Shit::TypeInfo *ti : comps) {
        auto *act = menu->addAction(QString::fromStdString(ti->name));
        connect(act, &QAction::triggered, this, [this, ti, target]() { addComponent(ti, target); });
    }
    return menu;
}

void SceneTree::addComponent(const Shit::TypeInfo *type, Shit::GameObject *target)
{
    if (!type || !target) return;
    void *raw = type->Create();   // factory 堆分配（非抽象）
    if (!raw) return;
    auto *comp = static_cast<Shit::Component *>(raw);
    target->addComponentInstance(comp);
    emit objectSelected(target);   // 刷新检查器显示新组件
}
