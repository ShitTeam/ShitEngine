#include "scenetree.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

SceneTree::SceneTree(QWidget *parent)
    : QWidget(parent)
    , m_view(new QTreeView(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    m_view->setHeaderHidden(true);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    // P3：连接 selectionChanged → objectSelected，供属性检查器联动
}

void SceneTree::setModel(QAbstractItemModel *model)
{
    m_view->setModel(model);
}
