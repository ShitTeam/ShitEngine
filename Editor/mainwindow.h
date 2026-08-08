#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QStringList>

#include <nlohmann/json.hpp>

#include "undostack.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCloseEvent;
class QMenu;
class QActionGroup;
class Viewport;
class SceneTree;
class Inspector;
class LogWidget;
class EnginePreview;
class AssetsDock;

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

protected:
    /// 关闭前未保存提示（保存/不保存/取消）
    void closeEvent(QCloseEvent *event) override;
    /// P12：播放态把运行视口的 Qt 输入事件转发给引擎（合成 SDL_Event）
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void newScene();
    void openScene();                    ///< 文件对话框打开（先未保存提示）
    bool saveScene();                    ///< 保存到当前路径（无路径则弹另存为）；返回是否成功
    bool saveSceneAs();                  ///< 另存为…
    void undo();                         ///< 撤销（Ctrl+Z）
    void redo();                         ///< 重做（Ctrl+Shift+Z）
    void about();
    void resetDockLayout();              ///< 恢复出厂默认 Dock 布局（P13）
    void setPlaying(bool playing);       ///< 播放/停止（▶/⏹）
    void pickSceneAt(float x, float y);  ///< 场景视图点击拾取
    void onViewportAssetDropped(const QString &path, float logicalX, float logicalY); ///< 资源图拖入视口 → 建精灵

private:
    void createDocks();
    void createMenus();
    void createToolbar();

    // ---- P8 编辑会话安全 ----
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

    // ---- P9 撤销/重做 ----
    void undoBegin();                                   ///< 开始一次编辑手势（运行态忽略）
    void undoCommit(const QString &label);              ///< 结束手势并提交（无差异不入栈）
    void applySnapshot(const nlohmann::json &snap);     ///< 用快照重建场景（保留编辑器相机）
    nlohmann::json snapshot() const;                    ///< 当前场景快照（排除编辑器相机）
    void refreshDirtyFromSaved();                       ///< dirty = 当前快照 ≠ 存档快照
    void updateUndoActions();                           ///< 撤销/重做菜单项可用性
    // ------------------------

    Ui::MainWindow *ui;

    Viewport *m_sceneViewport;
    Viewport *m_gameViewport;
    SceneTree *m_sceneTree;
    Inspector *m_inspector;
    LogWidget *m_log;
    EnginePreview *m_preview;   ///< 单一引擎预览（共享场景，双视图同源）
    QAction *m_playAction = nullptr;   ///< 播放/停止动作（createToolbar 创建；须默认初始化——createMenus 的 updateUndoActions 会先读 isPlaying()）
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QActionGroup *m_gizmoGroup = nullptr;   ///< Gizmo 模式互斥组（移动/旋转/缩放，Q/W/E）
    QMenu *m_recentMenu = nullptr;
    AssetsDock *m_assets = nullptr;

    bool m_dirty = false;       ///< 未保存修改标记（标题栏 *）
    QString m_scenePath;        ///< 当前场景文件路径（空 = 尚未保存过）
    QSettings m_settings;       ///< 最近场景等编辑器状态持久化
    UndoStack m_undo;           ///< 场景快照型撤销/重做栈（P9）
    nlohmann::json m_savedSnapshot;   ///< 最后保存/打开/新建时的场景快照（dirty 对比基准）
};
#endif // MAINWINDOW_H
