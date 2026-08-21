#ifndef SPRITEEDITORDIALOG_H
#define SPRITEEDITORDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QLabel;
class QSpinBox;

/// 精灵表网格参数
struct SpriteSheetParams {
    int rows = 1;
    int cols = 1;
    float frameWidth = 0.0f;
    float frameHeight = 0.0f;
    float margin = 0.0f;
    float spacing = 0.0f;
};

/// P38：精灵表配置对话框 — 输入行列/帧宽高/边距/间距，左侧纹理预览+网格叠加
class SpriteEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SpriteEditorDialog(const QString &imagePath, QWidget *parent = nullptr);

    SpriteSheetParams params() const;

private slots:
    void onGridChanged();

private:
    void autoCalcFrameSize();  ///< 按图片尺寸/行列/边距/间距自动推算帧宽高

    QLabel *m_preview;
    QSpinBox *m_rowsSpin;
    QSpinBox *m_colsSpin;
    QDoubleSpinBox *m_fwSpin;
    QDoubleSpinBox *m_fhSpin;
    QDoubleSpinBox *m_marginSpin;
    QDoubleSpinBox *m_spacingSpin;
    QImage m_image;
    QString m_imagePath;
    bool m_autoCalc = true;  ///< 自动推算模式（改行/列/边距/间距时自动更新帧宽高）
};

#endif // SPRITEEDITORDIALOG_H
