#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QWidget>

#include <functional>
#include <vector>

class QFormLayout;
class QScrollArea;

namespace Shit {
class GameObject;
class Component;
struct FieldInfo;
struct TypeInfo;
}

/// 右侧属性检查器：反射选中对象的组件字段，生成可编辑控件。
/// P4：TypeRegistry 查询字段元数据（displayName/range/step/readOnly），
/// 编辑通过 GetFieldPtr 就地写回组件；refresh() 每帧从引擎回读，实时同步。
class Inspector : public QWidget
{
    Q_OBJECT
public:
    explicit Inspector(QWidget *parent = nullptr);

    /// 清空并重建表单（无对象时显示占位）
    void clear();

    /// 反射 object 的组件并渲染为编辑表单
    void setGameObject(Shit::GameObject *object);

    /// 从组件重读当前值并更新控件（引擎 → 检查器实时同步）
    void refresh();

    /// 播放态编辑锁（P25d）：播放中所有控件只读（Unity 语义）——字段控件/Add Component/
    /// 移除按钮/对象名栏全部禁用，但每帧回读刷新照常（运行时值实时可见）。
    /// 停止后自动解锁；重建表单（setGameObject）时按当前状态重新应用。
    void setPlayMode(bool playing);

signals:
    /// 诊断：每次重建时报告渲染了 n 个组件 / n 个字段（供日志定位）
    void buildInfo(int components, int fields);
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

private:
    /// 为单个字段生成一行编辑控件
    void addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj);

    /// 只读字段/未知类型的字符串展示
    QString fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const;

    /// 底部"Add Component"按钮点击：弹出组件选择菜单
    void showAddComponentMenu();

    /// 向当前选中对象添加反射类型组件（工厂创建 + addComponentInstance）
    void addComponentToObject(const Shit::TypeInfo *type);

    /// 移除选中对象的某个组件（refuse 为 true 时先做基础设施拒删判断，拒绝则提示不执行）
    void removeComponentFromObject(Shit::Component *component);

    QScrollArea *m_scroll;
    QWidget *m_content;
    QFormLayout *m_form;

    /// 当前正在编辑的对象（nullptr = 未选中）；添加组件后需重建表单
    Shit::GameObject *m_object = nullptr;

    /// 每个字段的"组件 → 控件"回读函数（refresh 时逐行调用）
    std::vector<std::function<void()>> m_readbacks;

    int m_componentCount = 0;  ///< 本次构建渲染的组件数（诊断）
    int m_fieldCount = 0;      ///< 本次构建渲染的字段数（诊断）
    bool m_playMode = false;   ///< 播放态编辑锁（setPlayMode 写入，setGameObject 重建时重新应用）
    void applyEditLock();      ///< 按 m_playMode 统一禁用/启用表单控件
};

#endif // INSPECTOR_H
