#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QImage>
#include <QWidget>

class QMouseEvent;

/// 中央视口：显示引擎离屏渲染结果（P2），支持点击拾取。
class Viewport : public QWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent = nullptr);

    /// 更新显示一帧引擎渲染结果
    void setFrame(const QImage &frame);

signals:
    /// 点击视口（逻辑像素坐标，0..1280 × 0..720；供拾取用）
    void logicalClicked(float x, float y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QImage m_frame;
    QRect m_drawRect;   ///< 帧实际绘制区域（letterbox 居中后）
};

#endif // VIEWPORT_H