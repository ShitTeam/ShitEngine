#ifndef PREVIEW_H
#define PREVIEW_H

#include <QImage>
#include <QObject>
#include <QTimer>

#include <memory>
#include <vector>

namespace Shit { class EngineContext; class Scene; }

/// 预览视图模式（对齐 Unity/Godot 双视口）
enum class ViewMode {
    Scene,  ///< 预览场景：编辑器相机，满窗口看场景全貌
    Game,   ///< 运行页面：游戏相机，居中看玩家画面
};

/// 引擎预览：在独立的 EngineContext 中驱动引擎渲染（隐藏窗口离屏），
/// 每帧读回渲染缓冲像素并发出 QImage，供对应视口显示。
class EnginePreview : public QObject
{
    Q_OBJECT
public:
    explicit EnginePreview(ViewMode mode, QObject *parent = nullptr);
    ~EnginePreview();

    /// 初始化预览引擎 + 构建测试场景，启动定时渲染。成功返回 true。
    bool start();
    void stop();

    /// 预览的当前场景（未启动/已停止返回 nullptr）
    Shit::Scene *getScene();

signals:
    /// 每帧渲染完成后发出（含最新画面）
    void frameReady(const QImage &image);

private slots:
    void tick();

private:
    /// 运行时生成一张棋盘格 BMP（测试场景用，避免依赖仓库资产）
    QString writeTestBmp() const;

    ViewMode m_mode;
    QTimer m_timer;
    std::unique_ptr<Shit::EngineContext> m_context;
    std::vector<uint8_t> m_pixels;
    int m_logicalWidth = 0;
    int m_logicalHeight = 0;
    bool m_running = false;
};

#endif // PREVIEW_H