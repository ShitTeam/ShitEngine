#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QWidget>

class QFormLayout;
class QScrollArea;

/// 右侧属性检查器：展示选中对象的组件字段。
/// P4 起用反射（TypeRegistry）按 FieldMeta 动态生成编辑控件；
/// 此处先提供空表单骨架。
class Inspector : public QWidget
{
    Q_OBJECT
public:
    explicit Inspector(QWidget *parent = nullptr);

    /// 清空并重建表单（P4 起：根据选中对象的组件反射字段填充）
    void clear();

private:
    QScrollArea *m_scroll;
    QWidget *m_content;
    QFormLayout *m_form;
};

#endif // INSPECTOR_H
