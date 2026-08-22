#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QStringList>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

#include "project.h"
#include "scriptbuilder.h"
#include "undostack.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCloseEvent;
class QMenu;
class QActionGroup;
class QDockWidget;
class QTabWidget;
class QLabel;
class Viewport;
class SceneTree;
class Inspector;
class LogWidget;
class EnginePreview;
class AssetsDock;
class TilesetDock;
class AnimatorDock;
class AnimationDock;
class SpriteSheetDock;

namespace Shit { class Scene; class GameObject; }   ///< 播放中场景同步成员用（仅指针，不要求完整类型）

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// 当前场景是否含未保存修改（标题栏带 *）
    bool isDirty() const { return m_dirty; }
    /// 是否处于运行态播放（播放中编辑不记录撤销）
    bool isPlaying() const { return m_playAction && m_playAction->isChecked(); }
    /// 当前是否有打开的项目（无项目时编辑功能禁用）
    bool hasProject() const { return m_project.isValid(); }
    /// 命令行入口——`--project <dir>` 打开项目；`<xxx.scene>` 打开场景
    /// （scene 在项目目录内时连带打开该项目，供 .scene 文件关联使用）
    void openFromCommandLine(const QString &projectDir, const QString &sceneFile);

protected:
    /// 关闭前未保存提示（保存/不保存/取消）
    void closeEvent(QCloseEvent *event) override;
    /// 播放态把运行视口的 Qt 输入事件转发给引擎（合成 SDL_Event）
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void newScene();
    void openScene();                    ///< 文件对话框打开（先未保存提示）
    bool saveScene();                    ///< 保存到当前路径（无路径则弹另存为）；返回是否成功
    bool saveSceneAs();                  ///< 另存为…
    void undo();                         ///< 撤销（Ctrl+Z）
    void redo();                         ///< 重做（Ctrl+Shift+Z）
    void copySelectedObject();           ///< 复制选中对象到内部剪贴板（Ctrl+C；文本控件获焦时不劫持）
    void pasteObject();                  ///< 粘贴剪贴板对象为选中对象兄弟（Ctrl+V）
    void duplicateObject();              ///< 原地复制选中对象（Ctrl+D；复用内部剪贴板粘贴路径）
    void updateStatusInfo();             ///< 刷新状态栏常驻信息（选中对象 + 鼠标世界坐标）
    void about();
    void resetDockLayout();              ///< 恢复出厂默认 Dock 布局
    void saveLayoutAsDefault();          ///< 把当前 Dock 布局存为默认
    void setPlaying(bool playing);       ///< ▶ 运行 / ■ 停止（Unity 式：停止恢复运行前快照）
    void onPauseToggled(bool paused);    ///< ⏸ 暂停/继续（仅运行态可用）
    void onSceneFrameReady(const QImage &frame);  ///< 场景视图帧：同步树/选中态 + 检查器回读
    void pickSceneAt(float x, float y);  ///< 场景视图点击拾取
    void onViewportAssetDropped(const QString &path, float logicalX, float logicalY); ///< 资源图拖入视口 → 建精灵
    void onPrefabDropped(const QString &path, float logicalX, float logicalY); ///< .prefab 拖入视口 → 实例化
    /// 精灵帧拖入视口 → 创建带源矩形的精灵 GameObject（cols = .sprite 元数据列数；落点逻辑像素）
    void onViewportSpriteFrameDropped(const QString &texturePath, int frameIndex,
                                      float frameWidth, float frameHeight, float margin, float spacing,
                                      int cols, float logicalX, float logicalY);
    void onPrefabOpenRequested(const QString &path);  ///< 资产面板双击 .prefab → 实例化
    void onAnimOpenRequested(const QString &path);    ///< 资产面板双击 .anim → 应用剪辑到选中对象 Animator 状态
    void reloadAnimatorAsset(const QString &path);    ///< 方案 A：Animation 窗口保存 .anim → 同步引用该资产的 Animator 状态
    /// 从 .prefab 文件实例化进当前场景（useDropPos 时把根对象移到落点世界坐标）
    void instantiatePrefab(const QString &path, bool useDropPos, float logicalX, float logicalY);

    // 项目
    void newProject();                   ///< 新建项目向导
    void openProject();                  ///< 选择项目目录打开
    void closeProject();                 ///< 关闭当前项目（回到无项目态）
    void onProjectSettings();            ///< 项目设置（SDK 目录等）
    void onExportGame();                 ///< 导出游戏：装配绿色免安装游戏包
    void openIde();                      ///< 用项目设置中配置的 IDE 打开项目
    void onBuildScripts();               ///< 构建脚本工程（Ctrl+B）→ 成功后热重载
    void openAnimatorEditor();           ///< 显示并聚焦 Animator 状态机窗口（方案 A：检查器入口按钮触发）
    void openAnimationEditor();          ///< 显示并聚焦 Animation 帧动画窗口（检查器入口按钮触发）
    void onAnimationOpenRequested(const QString &path);  ///< 打开 .anim 到 Animation 窗口并聚焦

private:
    void createDocks();
    void createMenus();
    void createToolbar();

    // ---- 编辑会话安全 ----
    void setDirty(bool dirty);                          ///< 置/清 dirty 并刷新标题栏
    void updateWindowTitle();                           ///< "「场景名」* - ShitEngine 编辑器"
    bool confirmDiscardChanges(const QString &actionName); ///< 未保存提示；true=可继续
    bool openScenePath(const QString &path);            ///< 打开具体文件（失败回滚，成功返回 true）
    bool saveSceneTo(const QString &path);              ///< 写入指定路径
    void rollbackScene(const nlohmann::json &snapshot); ///< 清空当前场景并从快照整体恢复
    QStringList recentScenes() const;                   ///< 最近场景（自动剔除已删除文件）
    void addRecentScene(const QString &path);           ///< 记录最近场景（去重前插，超限截断）
    void updateRecentMenu();                            ///< 重建"最近场景"子菜单
    // ------------------------

    // ---- 撤销/重做 ----
    void undoBegin();                                   ///< 开始一次编辑手势（运行态忽略）
    void undoCommit(const QString &label);              ///< 结束手势并提交（无差异不入栈）
    void applySnapshot(const nlohmann::json &snap);     ///< 用快照重建场景（保留编辑器相机）
    nlohmann::json snapshot() const;                    ///< 当前场景快照（排除编辑器相机）
    void refreshDirtyFromSaved();                       ///< dirty = 当前快照 ≠ 存档快照
    void updateUndoActions();                           ///< 撤销/重做菜单项可用性
    // ------------------------

    // ---- 批量编辑 ----
    void syncInspectorToSelection();                    ///< 按选中数量切换检查器模式（>1 批量 / 1 单选）
    // ------------------------

    // ---- 项目 ----
    bool openProjectPath(const QString &path, bool silent = false); ///< 打开项目目录（成功 true）
    void enterProject(const Project &project);          ///< 切换到项目态（状态/插件/场景/布局）
    void closeProjectInternal();                        ///< 关闭项目（已过未保存提示）
    QSettings *stateSettings() const;                   ///< 项目级 .shitengine/state.ini；无项目回退全局
    void saveProjectState();                            ///< 布局/几何 → stateSettings()
    QStringList recentProjects() const;                 ///< 最近项目（全局，剔除已删目录）
    void addRecentProject(const QString &path);         ///< 记录最近项目（去重前插，超限截断）
    void updateRecentProjectsMenu();                    ///< 重建"最近项目"子菜单
    void updateProjectMenus();                          ///< 无项目时禁用编辑相关菜单/工具
    void applyInputMappingsToEngine();                  ///< 项目 inputMappings → 引擎热编译
    void resetToEmptyScene();                           ///< 清空场景（保留 scene_camera/game_camera）
    void onBuildFinished(bool success);                 ///< 构建结束：成功后触发插件热重载
    void enterPlayMode();                               ///< 进入运行态（引擎跑逻辑、暂停按钮启用）
    void exitPlayMode();                                ///< 停止运行：恢复运行前快照（Unity 语义）
    // ------------------------

    QByteArray m_factoryLayout;                     ///< 出厂默认 Dock 布局（createDocks 后捕获，恢复兜底）
    Ui::MainWindow *ui;

    Viewport *m_sceneViewport;
    Viewport *m_gameViewport;
    SceneTree *m_sceneTree;
    Inspector *m_inspector;
    LogWidget *m_log;
    EnginePreview *m_preview;   ///< 单一引擎预览（共享场景，双视图同源）

    // 项目
    Project m_project;              ///< 当前项目（isValid() 假 = 无项目态）
    QSettings *m_projectSettings = nullptr;  ///< 项目级状态（.shitengine/state.ini，IniFormat）
    QMenu *m_recentProjectsMenu = nullptr;
    QAction *m_newSceneAction = nullptr;
    QAction *m_openSceneAction = nullptr;
    QAction *m_saveSceneAction = nullptr;
    QAction *m_saveSceneAsAction = nullptr;
    QAction *m_closeProjectAction = nullptr;
    QAction *m_projectSettingsAction = nullptr;
    QAction *m_exportGameAction = nullptr; ///< 导出游戏
    QAction *m_openIdeAction = nullptr;  ///< 打开代码（Ctrl+Shift+O；IDE 经项目设置配置）
    QAction *m_buildAction = nullptr;    ///< 构建脚本（Ctrl+B）
    ScriptBuilder *m_scriptBuilder = nullptr;   ///< 脚本工程 cmake 编译管线

    QAction *m_playAction = nullptr;   ///< ▶ 运行 / ■ 停止（createToolbar 创建；须默认初始化——createMenus 的 updateUndoActions 会先读 isPlaying()）
    QAction *m_pauseAction = nullptr;  ///< ⏸ 暂停/继续（仅运行态启用；Ctrl+P 同效）
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QTabWidget *m_viewTabs = nullptr;  ///< 中央「场景视口/运行视口」标签页（播放自动切页）
    QLabel *m_statusInfo = nullptr;    ///< 状态栏常驻信息（选中对象 + 鼠标世界坐标）
    float m_mouseWorldX = 0.0f;        ///< 场景视口鼠标世界坐标（mouseWorldMoved 信号更新）
    float m_mouseWorldY = 0.0f;
    std::vector<QAction *> m_gizmoShortcutActions;   ///< Gizmo 三模式窗口快捷键（Q/W/E，不可见；运行态禁用防抢游戏键）
    QMenu *m_recentMenu = nullptr;
    AssetsDock *m_assets = nullptr;
    TilesetDock *m_tileset = nullptr;   ///< 瓦片选择面板：选中 Tilemap 时显示瓦片网格
    SpriteSheetDock *m_spriteSheetDock = nullptr;  ///< 精灵表视图 Dock
    AnimatorDock *m_animatorDock = nullptr;  ///< 状态机窗口：Unity 风格可视化状态机编辑
    AnimationDock *m_animationDock = nullptr; ///< 帧动画窗口：Unity 风格 .anim 资产编辑器
    std::vector<QDockWidget *> m_docks;   ///< 全部 Dock 面板（「窗口」菜单勾选显隐，关闭后可重新打开）

    bool m_dirty = false;       ///< 未保存修改标记（标题栏 *）
    QString m_scenePath;        ///< 当前场景文件路径（空 = 尚未保存过）
    QSettings m_settings;       ///< 全局编辑器状态（注册表：最近项目 / lastProjectDir / lastSdkDir）
    UndoStack m_undo;           ///< 场景快照型撤销/重做栈
    nlohmann::json m_savedSnapshot;   ///< 最后保存/打开/新建时的场景快照（dirty 对比基准）
    nlohmann::json m_clipboard;       ///< 复制/粘贴内部剪贴板（Prefab JSON；is_null() = 空）

    // ---- 运行态（Unity 式）----
    nlohmann::json m_runSnapshot;           ///< 进入运行前的场景快照（停止时恢复）
    bool m_hasRunSnapshot = false;          ///< 是否有待恢复的运行前快照
    bool m_playPendingBuild = false;        ///< 点运行后正等待脚本构建完成再进入
    // ----------------------------------

    // ---- 播放中场景同步（防悬垂）----
    Shit::Scene *m_lastScene = nullptr;     ///< 上次已同步的场景（比地址判断是否整体替换）
    uint64_t m_lastSceneGeneration = 0;     ///< 上次已同步的场景结构代数
    void syncSceneSelection();              ///< 场景结构变化 → 重建树/校验选中态/重绑检查器
    // ----------------------------------
};
#endif // MAINWINDOW_H