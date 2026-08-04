#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>

/// 中央视口：游戏场景的渲染区域。
/// P2 起在此接入引擎预览（EngineContext 离屏渲染 / 独立 SDL 窗口）。
class Viewport : public QWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // VIEWPORT_H
