#include "spriteeditordialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QPainter>

SpriteEditorDialog::SpriteEditorDialog(const QString &imagePath, QWidget *parent)
    : QDialog(parent)
    , m_preview(new QLabel(this))
    , m_rowsSpin(new QSpinBox(this))
    , m_colsSpin(new QSpinBox(this))
    , m_fwSpin(new QDoubleSpinBox(this))
    , m_fhSpin(new QDoubleSpinBox(this))
    , m_marginSpin(new QDoubleSpinBox(this))
    , m_spacingSpin(new QDoubleSpinBox(this))
    , m_imagePath(imagePath)
{
    setWindowTitle(tr("定义精灵表"));
    setMinimumSize(600, 450);
    resize(700, 500);

    m_image = QImage(imagePath);
    if (m_image.isNull()) {
        m_preview->setText(tr("无法加载图片：\n%1").arg(imagePath));
    } else {
        // 自动推算行列：默认均分为 1 行 1 列（完整图片）
        m_rowsSpin->setRange(1, 64);
        m_colsSpin->setRange(1, 64);
        m_rowsSpin->setValue(1);
        m_colsSpin->setValue(1);
        const float w = static_cast<float>(m_image.width());
        const float h = static_cast<float>(m_image.height());
        m_fwSpin->setRange(1.0, 10000.0);
        m_fhSpin->setRange(1.0, 10000.0);
        m_fwSpin->setDecimals(1);
        m_fhSpin->setDecimals(1);
        m_fwSpin->setValue(w);
        m_fhSpin->setValue(h);
    }

    m_marginSpin->setRange(0.0, 1000.0);
    m_marginSpin->setDecimals(1);
    m_marginSpin->setValue(0.0);
    m_spacingSpin->setRange(0.0, 1000.0);
    m_spacingSpin->setDecimals(1);
    m_spacingSpin->setValue(0.0);

    m_preview->setMinimumSize(200, 200);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet("QLabel { background: #1c2430; border: 1px solid #3a4a5a; border-radius: 4px; }");

    auto *gridLayout = new QGridLayout;
    gridLayout->addWidget(new QLabel(tr("行数：")), 0, 0);
    gridLayout->addWidget(m_rowsSpin, 0, 1);
    gridLayout->addWidget(new QLabel(tr("列数：")), 0, 2);
    gridLayout->addWidget(m_colsSpin, 0, 3);
    gridLayout->addWidget(new QLabel(tr("帧宽度：")), 1, 0);
    gridLayout->addWidget(m_fwSpin, 1, 1);
    gridLayout->addWidget(new QLabel(tr("帧高度：")), 1, 2);
    gridLayout->addWidget(m_fhSpin, 1, 3);
    gridLayout->addWidget(new QLabel(tr("边距：")), 2, 0);
    gridLayout->addWidget(m_marginSpin, 2, 1);
    gridLayout->addWidget(new QLabel(tr("间距：")), 2, 2);
    gridLayout->addWidget(m_spacingSpin, 2, 3);

    auto *paramsGroup = new QGroupBox(tr("网格参数"));
    paramsGroup->setLayout(gridLayout);

    auto *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(paramsGroup);
    rightLayout->addStretch(1);

    auto *totalLabel = new QLabel;
    rightLayout->addWidget(totalLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    rightLayout->addWidget(buttons);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_preview, 2);
    mainLayout->addLayout(rightLayout);

    connect(m_rowsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);
    connect(m_colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);
    connect(m_fwSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);
    connect(m_fhSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);
    connect(m_marginSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);
    connect(m_spacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteEditorDialog::onGridChanged);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onGridChanged();
}

SpriteSheetParams SpriteEditorDialog::params() const
{
    return {
        m_rowsSpin->value(),
        m_colsSpin->value(),
        static_cast<float>(m_fwSpin->value()),
        static_cast<float>(m_fhSpin->value()),
        static_cast<float>(m_marginSpin->value()),
        static_cast<float>(m_spacingSpin->value())
    };
}

void SpriteEditorDialog::onGridChanged()
{
    if (m_image.isNull()) return;

    const int rows = m_rowsSpin->value();
    const int cols = m_colsSpin->value();
    const float fw = m_fwSpin->value();
    const float fh = m_fhSpin->value();
    const float margin = m_marginSpin->value();
    const float spacing = m_spacingSpin->value();

    // 预览：缩放图片后叠加网格线
    QPixmap pix = QPixmap::fromImage(m_image.scaled(m_preview->width() - 8, m_preview->height() - 8,
                                                     Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QPainter painter(&pix);
    painter.setPen(QPen(QColor(122, 192, 255, 180), 1));
    const float scaleX = static_cast<float>(pix.width()) / static_cast<float>(m_image.width());
    const float scaleY = static_cast<float>(pix.height()) / static_cast<float>(m_image.height());
    for (int r = 0; r <= rows; ++r) {
        const float y = (margin + r * (fh + spacing)) * scaleY;
        painter.drawLine(0, static_cast<int>(y), pix.width(), static_cast<int>(y));
    }
    for (int c = 0; c <= cols; ++c) {
        const float x = (margin + c * (fw + spacing)) * scaleX;
        painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), pix.height());
    }
    m_preview->setPixmap(pix);
}
