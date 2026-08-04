#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QImage>
#include <QWidget>

/// 中央视口：显示引擎离屏渲染结果（P2）。
class Viewport : public QWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent = nullptr);

    /// 更新显示一帧引擎渲染结果
    void setFrame(const QImage &frame);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_frame;
};

#endif // VIEWPORT_H
