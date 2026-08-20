#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <ShitEngine/Scene/Scene.h>

#include <QWidget>

#include <functional>
#include <string>
#include <vector>

class QFormLayout;
class QScrollArea;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QWidget;
class QVBoxLayout;

namespace Shit {
class GameObject;
class Component;
class Scene;
class System;
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

    /// 设置当前场景（供场景系统面板在无选中对象时显示）
    void setScene(Shit::Scene *scene);

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

protected:
    // ── 面板级文件拖拽（P31）：资源面板/文件管理器拖文件到属性面板，
    // 按扩展名语义自动填充匹配的路径类 string 字段 ──
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    /// 找该文件可自动填充的字段（std::string 非只读 + 扩展名→字段名关键字匹配；
    /// 扩展名不在已知表时仅当对象只有一个候选字段才兜底）。返回 false 表示无匹配。
    bool findFileDropTarget(const QString &filePath, Shit::Component **outComp,
                            Shit::FieldInfo *outField) const;

    /// 拖拽悬停高亮开关（QSS 属性选择器）
    void setDropActive(bool on);
    bool m_dropActive = false;

    // ── P34：组件/字段搜索过滤 ──
    /// 一个组件渲染的 QFormLayout 行区间（组件头行..最后一个字段行）+ 搜索关键字
    struct ComponentSection {
        int startRow = -1;
        int endRow = -1;
        QString searchKey;
    };
    void applyFilter();   ///< 按 m_searchEdit 文本显隐组件行区间（清空恢复全显）
    std::vector<ComponentSection> m_sections;

    QScrollArea *m_scroll;
    QLineEdit *m_searchEdit = nullptr;   ///< P34：组件/字段搜索框（表单顶部，不随重建移除）
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

    // ── 场景系统面板 ──
    void buildSceneSystemPanel();  ///< 在 clear() 未选中态时构建场景系统面板
    void rebuildSystemPanel();     ///< refresh 时按签名重建系统面板
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
    int m_systemFieldCount = 0;          ///< 系统字段数（诊断）
};

#endif // INSPECTOR_H
