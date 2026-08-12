#ifndef SCENETREE_H
#define SCENETREE_H

#include <QWidget>

class QTreeView;
class QMenu;
class SceneTreeModel;

namespace Shit { class Scene; class GameObject; struct TypeInfo; }

/// 左侧场景树：列出当前场景的 GameObject 层级，选中联动属性检查器。
/// 右键菜单：新建对象 / 添加组件 / 删除对象。
class SceneTree : public QWidget
{
    Q_OBJECT
public:
    explicit SceneTree(QWidget *parent = nullptr);

    /// 绑定场景并填充层级（nullptr 清空）。autoSelect=true 时自动选中第一项
    /// （打开/新建场景等初次绑定用）；false 供播放中每帧结构同步（保留/恢复选中态）
    void setScene(Shit::Scene *scene, bool autoSelect = true);

    /// 程序化选中对象（供视口拾取联动），会触发 objectSelected
    void selectObject(Shit::GameObject *object);

    /// 当前选中对象（无选中/索引失效时返回 nullptr；仅地址比较，调用方需自行校验存活）
    Shit::GameObject *selectedObject() const;

signals:
    /// 选中某个对象（供属性检查器联动）
    void objectSelected(Shit::GameObject *object);
    /// 场景结构将被修改（新建/删除对象、添加组件发生前 → 撤销 begin）
    void sceneActionStarted();
    /// 场景结构被修改（新建/删除对象、添加组件 → 会话 dirty / 撤销提交）
    void sceneEdited();
    /// 删除被拒绝（如编辑器/游戏相机是基础设施，不允许删除）
    void sceneDeleteBlocked(const QString &reason);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void createObject();
    void deleteObject();
    void addComponent(const Shit::TypeInfo *type, Shit::GameObject *target);

    QTreeView *m_view;
    SceneTreeModel *m_model;
    Shit::Scene *m_scene = nullptr;
};

#endif // SCENETREE_H