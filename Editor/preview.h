#ifndef PREVIEW_H
#define PREVIEW_H

#include <QImage>
#include <QObject>
#include <QTimer>

#include <functional>
#include <memory>
#include <vector>

namespace Shit { class EngineContext; class Scene; class CameraComponent; class PluginManager; }

/// 引擎预览：单一 EngineContext + 单一场景，含编辑器相机（scene_camera 约定名）
/// 与游戏相机（任意名称，从场景启用相机中挑选，Unity 语义）。
/// 每帧两次渲染 pass（各只渲一个相机）→ 读回两个逻辑尺寸画面，分别供场景/运行视口。
/// 同一场景 ⇒ 编辑一处，两个视图同步反映。
/// 启动时从 exe 同目录 config.json 加载插件（脚本库），自定义行为类型可实例化/编辑/序列化。
class EnginePreview : public QObject
{
    Q_OBJECT
public:
    explicit EnginePreview(QObject *parent = nullptr);
    ~EnginePreview();

    /// 初始化预览引擎 + 加载插件 + 构建测试场景，启动定时渲染。成功返回 true。
    bool start();
    void stop();

    /// 预览的当前场景（共享，编辑器交互/场景树/序列化都用它）
    Shit::Scene *getScene();

    /// 预览引擎上下文（播放态输入转发需要 setCurrent）
    Shit::EngineContext *context() const { return m_context.get(); }

    /// 运行状态（start 后 true）
    bool isRunning() const { return m_running; }

    /// 切换项目插件集 —— 先清空场景对象（组件析构调用 DLL 内代码，须在
    /// UnloadAll 前完成），再卸载旧插件，最后从项目 config.json 加载新插件。
    /// configPath 不存在或没有 plugins 时仅完成卸载（不视为失败）。
    bool loadProjectConfig(const QString &configPath);

    /// 卸载项目插件（回到引擎内置类型集合）；场景对象先清空。
    void unloadPlugins();

    /// 热重载 —— 场景 JSON 快照 → 销毁场景对象（旧 DLL 代码析构完毕）
    /// → 卸载旧插件 →（可选 onDllReleased：此时旧 DLL 已无文件锁，可覆盖替换）
    /// → 从项目 config 重载新 DLL → 注册 → 快照恢复场景。
    /// 引擎会话与编辑器现场保持不变；onDllReleased 返回 false 则中止加载并恢复快照。
    /// 成功返回 true。
    bool reloadProjectPlugins(const QString &configPath,
                              const std::function<bool()> &onDllReplaced = {});

    /// 设置运行状态：true=引擎逻辑运行，false=暂停（画面静止）
    void setPlaying(bool playing);

    /// 单步：暂停态下请求推进一帧（下一 tick 临时解除暂停，帧末恢复暂停）
    void singleStep() { m_stepRequested = true; }

signals:
    /// 场景视图帧（编辑器相机，满窗）
    void sceneFrameReady(const QImage &image);
    /// 运行视图帧（游戏相机，居中）
    void gameFrameReady(const QImage &image);
    /// 引擎 spdlog 日志转发（isCore=引擎/用户日志；level=spdlog 等级；message=文本）
    void engineLogMessage(bool isCore, int level, const QString &message);
    /// 插件加载失败（DLL 缺失/ABI 不匹配等，detail 为引擎侧失败描述），
    /// 由 mainwindow 弹窗提示用户（延迟到事件循环空闲，避开启动期）
    void pluginLoadFailed(const QString &detail);

private slots:
    void tick();

private:
    /// 重新定位相机：编辑器相机按约定名 scene_camera；游戏相机取场景中
    /// priority 最小的非编辑器相机（上一帧选择仍在场景则延续），无则兜底编辑器相机
    void refreshCameras();
    /// 缺失的编辑器相机补齐（误删/播放中游戏逻辑销毁后自愈，保证场景视图不断流；
    /// 游戏相机不定名，无需补建，由 refreshCameras 挑选）
    void ensureCameras();
    /// 清空场景全部对象（保留编辑器相机 scene_camera；组件析构调用 DLL 代码）
    void clearSceneObjects();

    QTimer m_timer;
    std::unique_ptr<Shit::EngineContext> m_context;
    std::unique_ptr<Shit::PluginManager> m_plugins;   ///< 插件（脚本库）加载器
    Shit::Scene *m_scene = nullptr;            ///< 当前场景（SceneManager 持有）
    Shit::CameraComponent *m_editorCam = nullptr;
    Shit::CameraComponent *m_gameCam = nullptr;
    std::vector<uint8_t> m_pixels;
    int m_logicalWidth = 0;
    int m_logicalHeight = 0;
    bool m_running = false;
    bool m_stepRequested = false;   ///< 单步请求（Ctrl+Shift+P：暂停态推进一帧）
};

#endif // PREVIEW_H