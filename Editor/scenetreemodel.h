#ifndef SCENETREEMODEL_H
#define SCENETREEMODEL_H

#include <QAbstractItemModel>
#include <QByteArray>

#include <vector>

class QMimeData;

namespace Shit { class Scene; class GameObject; }

/// 场景层级树模型：包装 scene->getGameObjects() 的父子层级。
/// 支持重命名（setData/EditRole）与拖拽改层级（InternalMove + 自定义 mime 行号路径）。
class SceneTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SceneTreeModel(QObject *parent = nullptr);

    /// 绑定场景并重建（nullptr 表示空树）
    void setScene(Shit::Scene *scene);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 拖拽改层级（InternalMove）
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;

    /// index → GameObject*（无效返回 nullptr）
    Shit::GameObject *gameObjectAt(const QModelIndex &index) const;

    /// 按对象查找模型索引（不存在返回无效 index）
    QModelIndex indexOf(Shit::GameObject *object) const;

signals:
    /// 模型内数据被编辑（重命名 / 拖拽改层级 → 会话 dirty + 撤销提交）
    void dataEdited();

private:
    /// 父节点（无效 = 根）下的子对象列表
    std::vector<Shit::GameObject *> childrenOf(const QModelIndex &parent) const;
    /// 顶层对象（无父对象）
    std::vector<Shit::GameObject *> topLevelObjects() const;

    /// 在兄弟列表中递归查找目标（含子树）
    QModelIndex indexOfIn(const std::vector<Shit::GameObject *> &siblings,
                          Shit::GameObject *target, const QModelIndex &parentIdx) const;

    /// 编码 index 的"根到节点行号路径"（mime 传输；拖拽期间结构不变，路径稳定）
    static QByteArray encodePath(const QModelIndex &index);
    /// 按编码路径定位对象（任一级越界/失效返回 nullptr）
    Shit::GameObject *objectFromPath(const QByteArray &path) const;

    Shit::Scene *m_scene = nullptr;
};

#endif // SCENETREEMODEL_H
