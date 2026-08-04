#ifndef SCENETREEMODEL_H
#define SCENETREEMODEL_H

#include <QAbstractItemModel>

#include <vector>

namespace Shit { class Scene; class GameObject; }

/// 场景层级树模型：包装 scene->getGameObjects() 的父子层级。
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

    /// index → GameObject*（无效返回 nullptr）
    Shit::GameObject *gameObjectAt(const QModelIndex &index) const;

    /// 按对象查找模型索引（不存在返回无效 index）
    QModelIndex indexOf(Shit::GameObject *object) const;

private:
    /// 父节点（无效 = 根）下的子对象列表
    std::vector<Shit::GameObject *> childrenOf(const QModelIndex &parent) const;
    /// 顶层对象（无父对象）
    std::vector<Shit::GameObject *> topLevelObjects() const;

    /// 在兄弟列表中递归查找目标（含子树）
    QModelIndex indexOfIn(const std::vector<Shit::GameObject *> &siblings,
                          Shit::GameObject *target, const QModelIndex &parentIdx) const;

    Shit::Scene *m_scene = nullptr;
};

#endif // SCENETREEMODEL_H
