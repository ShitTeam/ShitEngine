#ifndef SCENETREE_H
#define SCENETREE_H

#include <QWidget>

class QTreeView;
class QMenu;
class SceneTreeModel;

namespace Shit { class Scene; class GameObject; class Component; struct TypeInfo; }

/// 左侧场景树：列出当前场景的 GameObject 层级，选中联动属性检查器。
/// 右键菜单：新建对象 / 添加组件 / 删除对象。
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
    /// 场景结构将被修改（新建/删除对象、添加组件发生前 → 撤销 begin）
    void sceneActionStarted();
    /// 场景结构被修改（新建/删除对象、添加组件 → 会话 dirty / 撤销提交）
    void sceneEdited();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void createObject();
    void deleteObject();
    /// 为指定对象生成"添加组件"子菜单
    QMenu *buildAddComponentMenu(Shit::GameObject *target);
    void addComponent(const Shit::TypeInfo *type, Shit::GameObject *target);

    QTreeView *m_view;
    SceneTreeModel *m_model;
    Shit::Scene *m_scene = nullptr;
};

#endif // SCENETREE_H