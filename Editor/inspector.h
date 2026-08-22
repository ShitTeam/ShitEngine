#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <ShitEngine/Scene/Scene.h>

#include <QString>
#include <QWidget>

#include <functional>
#include <string>
#include <typeindex>
#include <vector>

class QFormLayout;
class QScrollArea;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QVBoxLayout;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;

namespace Shit {
class GameObject;
class Component;
class Scene;
class System;
struct FieldInfo;
struct TypeInfo;
struct PropertyInfo;
}

/// 右侧属性检查器：反射选中对象的组件字段，生成可编辑控件。
/// TypeRegistry 查询字段元数据（displayName/range/step/readOnly），
/// 编辑通过 GetFieldPtr 就地写回组件；refresh() 每帧从引擎回读，实时同步。
class Inspector : public QWidget
{
    Q_OBJECT
public:
    explicit Inspector(QWidget *parent = nullptr);

    /// 清空并重建组件页表单（无对象时显示占位；系统页常驻独立，不受影响）
    void clear();

    /// 反射 object 的组件并渲染为编辑表单
    void setGameObject(Shit::GameObject *object);

    /// 从组件重读当前值并更新控件（引擎 → 检查器实时同步）
    void refresh();

    /// 播放态编辑锁：播放中所有控件只读（Unity 语义）——字段控件/Add Component/
    /// 移除按钮/对象名栏全部禁用，但每帧回读刷新照常（运行时值实时可见）。
    /// 停止后自动解锁；重建表单（setGameObject）时按当前状态重新应用。
    void setPlayMode(bool playing);

    /// 设置当前场景（场景整体替换时重建系统页；同一场景的代数变化不重建）
    void setScene(Shit::Scene *scene);

signals:
    /// 任一字段被编辑（控件写入引擎值 → 会话 dirty）
    void fieldEdited();
    /// 一次字段编辑结束（数值/文本控件 editingFinished，或按钮/下拉即时提交 → 撤销提交点）
    void fieldCommitted();
    /// 通过底部 Add Component 菜单向选中对象添加了组件（撤销提交点，标签"添加组件"）
    void componentAdded();
    /// 通过组件头「✕」按钮移除了组件（撤销提交点，标签"移除组件"）
    void componentRemoved();
    /// 通过顶部名称栏重命名了对象（撤销提交点，标签"重命名"）
    void objectRenamed();
    /// 组件移除被拒绝（如 Transform / scene_camera 相机是基础设施）
    void componentRemoveBlocked(const QString &reason);
    /// 用户请求打开 Animator 状态机编辑窗口（Animator 组件的入口按钮触发，由 mainwindow 显示 Dock）
    void openAnimatorEditorRequested();
    /// 用户请求打开 Animation 帧动画编辑窗口（AnimationComponent 的入口按钮触发，由 mainwindow 显示 Dock）
    void openAnimationEditorRequested();

    /// 通过场景系统面板添加了系统（撤销提交点，标签"添加系统"）
    void systemAdded();
    /// 通过场景系统面板移除了系统（撤销提交点，标签"移除系统"）
    void systemRemoved();
    /// 场景系统优先级被调整（撤销提交点，标签"调整系统优先级"）
    void systemPriorityChanged();

protected:
    // ── 面板级文件拖拽（P31）：文件拖到属性面板 → 自动填充匹配的路径字段 ──
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    /// 按扩展名语义在选中对象的 string 字段中找拖放目标（首个命中；未知扩展且
    /// 唯一 string 字段时兜底）。命中返回 true 并写出 comp/field
    bool findFileDropTarget(const QString& filePath, Shit::Component** outComp,
                            Shit::FieldInfo* outField) const;
    void setDropActive(bool on);   ///< 拖拽悬停时虚线高亮
    void applyFilter();            ///< 组件/字段搜索过滤（P34，按组件行区间显隐）

private:
    /// 为单个字段生成一行编辑控件
    void addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj);

    /// 为属性生成一行编辑控件（通过 getter/setter 读写）
    void addPropertyRow(const Shit::PropertyInfo &prop, Shit::Component *obj);

    /// 只读字段/未知类型的字符串展示
    QString fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const;

    /// 底部"Add Component"按钮点击：弹出组件选择菜单
    void showAddComponentMenu();

    /// 向当前选中对象添加反射类型组件（工厂创建 + addComponentInstance）
    void addComponentToObject(const Shit::TypeInfo *type);

    /// 移除选中对象的某个组件（refuse 为 true 时先做基础设施拒删判断，拒绝则提示不执行）
    void removeComponentFromObject(Shit::Component *component);

private:
    QTabWidget *m_tabs;             ///< 「组件 / 系统」两页顶层标签（选中对象自动切组件页）
    QScrollArea *m_scroll;
    QWidget *m_content;
    QFormLayout *m_form;

    QScrollArea *m_systemScroll;    ///< 系统页滚动容器（常驻，独立于组件页）
    QWidget *m_systemContent;
    QVBoxLayout *m_systemPageLayout;   ///< 系统页内容布局（m_scenePanel 挂载点）

    /// 当前正在编辑的对象（nullptr = 未选中，组件页显示占位）
    Shit::GameObject *m_object = nullptr;

    /// 每个字段的"组件 → 控件"回读函数（refresh 时逐行调用，组件页）
    std::vector<std::function<void()>> m_readbacks;
    /// 系统页字段的回读函数（与组件页分开清理，重建系统页不误杀组件页回读）
    std::vector<std::function<void()>> m_systemReadbacks;

    /// 组件在表单中的行区间（P34 搜索过滤：按区间整块显隐）
    struct SectionRange { int startRow; int endRow; QString searchKey; };
    std::vector<SectionRange> m_sections;
    QLineEdit* m_searchEdit = nullptr;   ///< 组件/字段搜索框（组件页顶部）
    bool m_dropActive = false;           ///< 文件拖拽悬停高亮状态

    int m_componentCount = 0;  ///< 本次构建渲染的组件数
    int m_fieldCount = 0;      ///< 本次构建渲染的字段数
    bool m_playMode = false;   ///< 播放态编辑锁（setPlayMode 写入，setGameObject 重建时重新应用）
    void applyEditLock();      ///< 按 m_playMode 统一禁用/启用表单控件

    // ── 场景系统页 ──
    void buildSceneSystemPanel();  ///< 构建场景系统面板（挂载到系统页 m_systemPageLayout）
    void rebuildSystemPanel();     ///< 场景替换 / 签名变化时重建系统页内容
    void showAddSystemMenu();      ///< 弹出添加系统菜单
    void addSystemToScene(const Shit::TypeInfo *type);  ///< 向场景添加系统
    void removeSystemFromScene(const std::string &typeName);  ///< 从场景移除系统
    void setSystemPriority(const std::string &typeName, int priority);  ///< 调整系统优先级
    void addSystemFieldRow(const Shit::FieldInfo &field, Shit::System *sys);  ///< 系统字段编辑行

    Shit::Scene *m_scene = nullptr;      ///< 当前场景（供系统面板使用）
    std::string m_systemsSignature;      ///< 系统列表签名（用于检测变化）
    QWidget *m_scenePanel = nullptr;     ///< 场景系统面板容器
    QVBoxLayout *m_systemLayout = nullptr; ///< 系统面板内布局
    QFormLayout *m_systemFieldsForm = nullptr; ///< 选中系统的字段编辑表单布局
    QString m_selectedSystemName;        ///< 当前选中的系统名（展开字段编辑）
    QWidget *m_systemFieldsContainer = nullptr; ///< 选中系统的字段编辑容器
    int m_systemFieldCount = 0;          ///< 系统字段数
};

#endif // INSPECTOR_H