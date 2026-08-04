#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QWidget>

class QFormLayout;
class QScrollArea;

namespace Shit {
class GameObject;
class Component;
struct FieldInfo;
}

/// 右侧属性检查器：反射选中对象的组件字段，生成可编辑控件。
/// P4：TypeRegistry 查询字段元数据（displayName/range/step/readOnly），
/// 编辑通过 GetFieldPtr 就地写回组件。
class Inspector : public QWidget
{
    Q_OBJECT
public:
    explicit Inspector(QWidget *parent = nullptr);

    /// 清空并重建表单（无对象时显示占位）
    void clear();

    /// 反射 object 的组件并渲染为编辑表单
    void setGameObject(Shit::GameObject *object);

private:
    /// 为单个字段生成一行编辑控件
    void addFieldRow(const Shit::FieldInfo &field, Shit::Component *obj);

    /// 只读字段/未知类型的字符串展示
    QString fieldToString(const Shit::FieldInfo &field, Shit::Component *obj) const;

    QScrollArea *m_scroll;
    QWidget *m_content;
    QFormLayout *m_form;
};

#endif // INSPECTOR_H
