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

private:
    /// 父节点（无效 = 根）下的子对象列表
    std::vector<Shit::GameObject *> childrenOf(const QModelIndex &parent) const;
    /// 顶层对象（无父对象）
    std::vector<Shit::GameObject *> topLevelObjects() const;

    Shit::Scene *m_scene = nullptr;
};

#endif // SCENETREEMODEL_H
