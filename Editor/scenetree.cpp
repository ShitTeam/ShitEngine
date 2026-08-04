#include "scenetree.h"

#include "scenetreemodel.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>

SceneTree::SceneTree(QWidget *parent)
    : QWidget(parent)
    , m_view(new QTreeView(this))
    , m_model(new SceneTreeModel(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    m_view->setModel(m_model);
    m_view->setHeaderHidden(true);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 选中 → objectSelected，供属性检查器联动
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) {
                emit objectSelected(m_model->gameObjectAt(current));
            });
}

void SceneTree::setScene(Shit::Scene *scene)
{
    m_model->setScene(scene);
    m_view->expandAll();

    // 自动选中第一项，触发 objectSelected → 检查器
    if (m_model->rowCount({}) > 0) {
        const QModelIndex first = m_model->index(0, 0, {});
        m_view->selectionModel()->setCurrentIndex(first,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void SceneTree::selectObject(Shit::GameObject *object)
{
    const QModelIndex idx = m_model->indexOf(object);
    if (idx.isValid()) {
        m_view->selectionModel()->setCurrentIndex(idx,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
        m_view->scrollTo(idx);
    }
}
