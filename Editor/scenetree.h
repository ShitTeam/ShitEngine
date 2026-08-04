#ifndef SCENETREE_H
#define SCENETREE_H

#include <QWidget>

class QAbstractItemModel;
class QTreeView;

/// 左侧场景树：列出当前场景的 GameObject 层级。
/// P3 起由场景数据填充模型；此处先提供结构。
class SceneTree : public QWidget
{
    Q_OBJECT
public:
    explicit SceneTree(QWidget *parent = nullptr);

    /// 设置场景对象模型（P3 接入 scene->getGameObjects() 的适配模型）
    void setModel(QAbstractItemModel *model);

signals:
    /// 选中某个对象（P3 起连接属性检查器）
    void objectSelected(const QString &objectName);

private:
    QTreeView *m_view;
};

#endif // SCENETREE_H
