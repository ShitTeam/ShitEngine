#ifndef SCENETREE_H
#define SCENETREE_H

#include <QWidget>

class QTreeView;
class SceneTreeModel;

namespace Shit { class Scene; class GameObject; }

/// 左侧场景树：列出当前场景的 GameObject 层级，选中联动属性检查器。
class SceneTree : public QWidget
{
    Q_OBJECT
public:
    explicit SceneTree(QWidget *parent = nullptr);

    /// 绑定场景并填充层级（nullptr 清空），自动选中第一项
    void setScene(Shit::Scene *scene);

    /// 程序化选中对象（供视口拾取联动），会触发 objectSelected
    void selectObject(Shit::GameObject *object);

signals:
    /// 选中某个对象（供属性检查器联动）
    void objectSelected(Shit::GameObject *object);

private:
    QTreeView *m_view;
    SceneTreeModel *m_model;
};

#endif // SCENETREE_H
