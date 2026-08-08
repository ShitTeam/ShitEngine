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

signals:
    /// 诊断：每次重建时报告渲染了 n 个组件 / n 个字段（供日志定位）
    void buildInfo(int components, int fields);
    /// 任一字段被编辑（控件写入引擎值 → 会话 dirty）
    void fieldEdited();
    /// 一次字段编辑结束（数值/文本控件 editingFinished，或按钮/下拉即时提交 → 撤销提交点）
    void fieldCommitted();

private:
    /// 为单个字段生成一行编辑控件
    void addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj);

    /// 只读字段/未知类型的字符串展示
    QString fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const;

    QScrollArea *m_scroll;
    QWidget *m_content;
    QFormLayout *m_form;

    /// 每个字段的"组件 → 控件"回读函数（refresh 时逐行调用）
    std::vector<std::function<void()>> m_readbacks;

    int m_componentCount = 0;  ///< 本次构建渲染的组件数（诊断）
    int m_fieldCount = 0;      ///< 本次构建渲染的字段数（诊断）
};

#endif // INSPECTOR_H
