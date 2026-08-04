#include "inspector.h"

#include <QFormLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

Inspector::Inspector(QWidget *parent)
    : QWidget(parent)
    , m_scroll(new QScrollArea(this))
    , m_content(new QWidget)
    , m_form(new QFormLayout(m_content))
{
    m_scroll->setWidget(m_content);
    m_scroll->setWidgetResizable(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scroll);

    m_form->setContentsMargins(8, 8, 8, 8);
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto *placeholder = new QLabel(tr("未选中对象\n\n选中场景中的对象后，\n在此编辑其组件属性。"), m_content);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setWordWrap(true);
    m_form->addRow(placeholder);
}

void Inspector::clear()
{
    // 清除所有行（P4 起重建为反射表单）
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);
}
