#include "preview.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

namespace {

/// 测试场景：让精灵在屏幕上往返移动（验证 BehaviorSystem 在编辑器里也被驱动）
class PreviewMover : public Shit::Behavior
{
public:
    void onUpdate() override
    {
        auto *transform = getOwner()->getComponent<Shit::TransformComponent>();
        if (!transform) return;
        Shit::Vector2 pos = transform->getPosition();
        pos.x += 80.0f * Shit::Time::GetDeltaTime();
        if (pos.x > 90.0f) pos.x = -90.0f;
        transform->setPosition(pos);
    }
};

} // namespace

EnginePreview::EnginePreview(ViewMode mode, QObject *parent)
    : QObject(parent)
    , m_mode(mode)
{
}

EnginePreview::~EnginePreview()
{
    stop();
}

bool EnginePreview::start()
{
    if (m_running) return true;

    m_context = std::make_unique<Shit::EngineContext>();
    Shit::EngineContext::setCurrent(m_context.get());

    Shit::Window::SetHidden(true); // 离屏渲染：隐藏窗口，渲染照常进行
    if (!Shit::Game::Init()) return false;

    // 构建测试场景：相机 + 一个移动的棋盘格精灵
    auto scene = std::make_unique<Shit::Scene>("preview");
    scene->init();

    auto *player = scene->createGameObject("player");
    player->addComponent<Shit::TransformComponent>();
    player->addComponent<Shit::SpriteRenderer>()->setTexturePath(writeTestBmp().toStdString());
    player->addComponent<PreviewMover>();

    // 按视图模式建相机：Scene=编辑器相机看全貌，Game=游戏相机看玩家
    auto *cam = scene->createGameObject("camera");
    cam->addComponent<Shit::TransformComponent>();
    cam->addComponent<Shit::CameraComponent>()->setZoom(m_mode == ViewMode::Scene ? 1.0f : 5.0f);

    Shit::SceneManager::LoadScene(std::move(scene));

    m_logicalWidth = Shit::Renderer::GetLogicalWidth();
    m_logicalHeight = Shit::Renderer::GetLogicalHeight();
    m_pixels.resize(static_cast<size_t>(m_logicalWidth) * m_logicalHeight * 4);

    connect(&m_timer, &QTimer::timeout, this, &EnginePreview::tick);
    m_timer.start(1000 / 60);
    m_running = true;
    return true;
}

void EnginePreview::stop()
{
    if (!m_running) return;
    m_timer.stop();
    if (m_context) {
        Shit::EngineContext::setCurrent(m_context.get());
        Shit::Game::Destroy();
        Shit::EngineContext::resetCurrent();
        m_context.reset();
    }
    m_running = false;
}

Shit::Scene *EnginePreview::getScene()
{
    if (!m_context) return nullptr;
    Shit::EngineContext::setCurrent(m_context.get());
    return Shit::SceneManager::GetCurrentScene();
}

void EnginePreview::tick()
{
    if (!m_context) return;
    Shit::EngineContext::setCurrent(m_context.get());

    // 复刻 Game::run() 的单帧逻辑（不阻塞 Qt 事件循环）
    Shit::Time::Update();
    Shit::EventBus::ProcessEvents();
    Shit::SceneManager::Update();
    Shit::Input::Update();
    Shit::AudioPlayer::Update();

    // 读回渲染缓冲 → QImage（ARGB8888 与 QImage::Format_ARGB32 字节序一致）
    if (Shit::Renderer::ReadPixels(m_pixels.data(), m_logicalWidth * 4)) {
        QImage image(m_pixels.data(), m_logicalWidth, m_logicalHeight,
                     m_logicalWidth * 4, QImage::Format_ARGB32);
        emit frameReady(image.copy()); // 深拷贝：m_pixels 下一帧会被覆盖
    }
}

QString EnginePreview::writeTestBmp() const
{
    // 生成 64x64 棋盘格 BMP（SDL3_image 原生支持 BMP，无需依赖仓库资产）
    const int w = 64, h = 64;
    const int rowBytes = ((w * 3 + 3) / 4) * 4;
    const int imageSize = rowBytes * h;
    const int fileSize = 54 + imageSize;

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.writeRawData("BM", 2);
    ds << quint32(fileSize) << quint32(0) << quint32(54);       // 文件头
    ds << quint32(40) << quint32(w) << quint32(h);              // 信息头
    ds << quint16(1) << quint16(24);                            // 平面数 + 位深
    ds << quint32(0) << quint32(imageSize) << quint32(0) << quint32(0) << quint32(0) << quint32(0);

    // 像素（自底向上，BGR），每 8 像素换色 → 棋盘格
    for (int y = h - 1; y >= 0; --y) {
        for (int x = 0; x < w; ++x) {
            const bool even = ((x / 8) + (y / 8)) % 2 == 0;
            ds << quint8(even ? 255 : 40)   // B
               << quint8(even ? 120 : 80)   // G
               << quint8(even ? 60 : 140);  // R
        }
        for (int pad = 0; pad < rowBytes - w * 3; ++pad)
            ds << quint8(0);
    }

    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                   + "/shitengine_preview.bmp";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
    return path;
}
