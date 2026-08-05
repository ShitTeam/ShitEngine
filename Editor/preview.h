#ifndef PREVIEW_H
#define PREVIEW_H

#include <QImage>
#include <QObject>
#include <QTimer>

#include <memory>
#include <vector>

namespace Shit { class EngineContext; class Scene; class CameraComponent; }

/// 引擎预览：单一 EngineContext + 单一场景，含编辑器相机与游戏相机。
/// 每帧两次渲染 pass（各只渲一个相机）→ 读回两个逻辑尺寸画面，分别供场景/运行视口。
/// 同一场景 ⇒ 编辑一处，两个视图同步反映。
class EnginePreview : public QObject
{
    Q_OBJECT
public:
    explicit EnginePreview(QObject *parent = nullptr);
    ~EnginePreview();

    /// 初始化预览引擎 + 构建测试场景，启动定时渲染。成功返回 true。
    bool start();
    void stop();

    /// 预览的当前场景（共享，编辑器交互/场景树/序列化都用它）
    Shit::Scene *getScene();

    /// 设置运行状态：true=引擎逻辑运行，false=暂停（画面静止）
    void setPlaying(bool playing);

signals:
    /// 场景视图帧（编辑器相机，满窗）
    void sceneFrameReady(const QImage &image);
    /// 运行视图帧（游戏相机，居中）
    void gameFrameReady(const QImage &image);

private slots:
    void tick();

private:
    /// 运行时生成一张棋盘格 BMP（测试场景用，避免依赖仓库资产）
    QString writeTestBmp() const;

    QTimer m_timer;
    std::unique_ptr<Shit::EngineContext> m_context;
    Shit::Scene *m_scene = nullptr;            ///< 当前场景（SceneManager 持有）
    Shit::CameraComponent *m_editorCam = nullptr;
    Shit::CameraComponent *m_gameCam = nullptr;
    std::vector<uint8_t> m_pixels;
    int m_logicalWidth = 0;
    int m_logicalHeight = 0;
    bool m_running = false;
};

#endif // PREVIEW_H