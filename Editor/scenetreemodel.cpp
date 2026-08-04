#include "scenetreemodel.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

SceneTreeModel::SceneTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

void SceneTreeModel::setScene(Shit::Scene *scene)
{
    beginResetModel();
    m_scene = scene;
    endResetModel();
}

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent) || !m_scene)
        return {};
    auto children = childrenOf(parent);
    if (row < 0 || row >= static_cast<int>(children.size()))
        return {};
    return createIndex(row, column, children[row]);
}

QModelIndex SceneTreeModel::parent(const QModelIndex &child) const
{
    auto *obj = gameObjectAt(child);
    if (!obj) return {};
    auto *p = obj->getParent();
    if (!p) return {};

    auto *gp = p->getParent();
    auto sibs = gp ? gp->getChildren() : topLevelObjects();
    for (size_t r = 0; r < sibs.size(); ++r)
        if (sibs[r] == p)
            return createIndex(static_cast<int>(r), 0, p);
    return {};
}

int SceneTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!m_scene) return 0;
    if (parent.isValid() && parent.column() > 0) return 0;
    return static_cast<int>(childrenOf(parent).size());
}

int SceneTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant SceneTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return {};
    auto *obj = gameObjectAt(index);
    if (!obj) return {};
    if (role == Qt::DisplayRole)
        return QString::fromStdString(obj->getName());
    return {};
}

Shit::GameObject *SceneTreeModel::gameObjectAt(const QModelIndex &index) const
{
    return static_cast<Shit::GameObject *>(index.internalPointer());
}

QModelIndex SceneTreeModel::indexOf(Shit::GameObject *object) const
{
    if (!m_scene || !object) return {};
    return indexOfIn(topLevelObjects(), object, {});
}

QModelIndex SceneTreeModel::indexOfIn(const std::vector<Shit::GameObject *> &siblings,
                                      Shit::GameObject *target, const QModelIndex &parentIdx) const
{
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i] == target)
            return index(static_cast<int>(i), 0, parentIdx);
        const QModelIndex childIdx = index(static_cast<int>(i), 0, parentIdx);
        const QModelIndex sub = indexOfIn(siblings[i]->getChildren(), target, childIdx);
        if (sub.isValid())
            return sub;
    }
    return {};
}

std::vector<Shit::GameObject *> SceneTreeModel::childrenOf(const QModelIndex &parent) const
{
    if (!m_scene) return {};
    if (!parent.isValid())
        return topLevelObjects();
    auto *obj = gameObjectAt(parent);
    return obj ? obj->getChildren() : std::vector<Shit::GameObject *>{};
}

std::vector<Shit::GameObject *> SceneTreeModel::topLevelObjects() const
{
    std::vector<Shit::GameObject *> roots;
    for (auto &go : m_scene->getGameObjects())
        if (go && !go->getParent())
            roots.push_back(go.get());
    return roots;
}