#include "scenetreemodel.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include "dnd.h"

#include <QColor>
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>

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
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return QString::fromStdString(obj->getName());
    // 失活对象灰显（Unity 同款，一眼可辨；isActiveInHierarchy 含父链级联状态）
    if (role == Qt::ForegroundRole && !obj->isActiveInHierarchy())
        return QColor(120, 120, 120);
    return {};
}

bool SceneTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid()) return false;
    auto *obj = gameObjectAt(index);
    if (!obj) return false;
    obj->setName(value.toString().toStdString());
    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    emit dataEdited();
    return true;
}

Qt::ItemFlags SceneTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (!index.isValid()) return f;
    return f | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions SceneTreeModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList SceneTreeModel::mimeTypes() const
{
    // 对象路径（层级拖拽）+ 组件引用列表（检查器引用字段拖入）
    return { QString::fromLatin1(kDndObjectPath), QString::fromLatin1(kDndComponentRef) };
}

QByteArray SceneTreeModel::encodePath(const QModelIndex &index)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    QList<int> rows;                   // 自根节点到目标路径（行号序列）
    QModelIndex cur = index;
    while (cur.isValid()) {
        rows.prepend(cur.row());
        cur = cur.parent();
    }
    stream << rows;
    return bytes;
}

QMimeData *SceneTreeModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty()) return nullptr;
    auto *mime = new QMimeData;
    const QByteArray path = encodePath(indexes.first());
    mime->setData(QString::fromLatin1(kDndObjectPath), path);

    // 同时附加组件引用列表（拖场景对象到检查器引用字段时，自动挑第一个可赋值的组件）
    QList<std::pair<quint64, QString>> refs;
    if (Shit::GameObject *go = objectFromPath(path)) {
        go->forEachComponent([&refs](Shit::Component *comp) {
            const Shit::TypeInfo *ti = Shit::TypeRegistry::Get(std::type_index(typeid(*comp)));
            refs.append({ comp->getUuid(),
                          ti ? QString::fromStdString(ti->name) : QString() });
        });
    }
    mime->setData(QString::fromLatin1(kDndComponentRef), encodeComponentRefs(refs));
    return mime;
}

Shit::GameObject *SceneTreeModel::objectFromPath(const QByteArray &path) const
{
    QDataStream stream(path);
    QList<int> rows;
    stream >> rows;
    if (stream.status() != QDataStream::Ok) return nullptr;

    QModelIndex cur;
    Shit::GameObject *obj = nullptr;
    for (const int row : rows) {
        const QModelIndex child = index(row, 0, cur);
        if (!child.isValid()) return nullptr;
        obj = gameObjectAt(child);
        cur = child;
    }
    return obj;
}

bool SceneTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (action == Qt::IgnoreAction) return true;
    if (!m_scene || !data || !data->hasFormat(QString::fromLatin1(kDndObjectPath)))
        return false;

    Shit::GameObject *source = objectFromPath(data->data(QString::fromLatin1(kDndObjectPath)));
    if (!source) return false;
    Shit::GameObject *target = gameObjectAt(parent);   // parent 无效 = 拖到根
    if (target == source) return false;

    // 防环：target 是 source 自身或其子孙时拒绝
    for (Shit::GameObject *p = target; p; p = p->getParent())
        if (p == source) return false;
    // 已在同一父下 → 无变化
    if (source->getParent() == target) return false;

    source->setParent(target);
    beginResetModel();
    endResetModel();                 // 层级已变，整体刷新
    emit dataEdited();
    return true;
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
        // 编辑器相机（scene_camera）是编辑基础设施，不出现在场景树
        if (go && !go->getParent() && go->getName() != "scene_camera")
            roots.push_back(go.get());
    return roots;
}