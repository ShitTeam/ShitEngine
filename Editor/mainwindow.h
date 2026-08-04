#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Viewport;
class SceneTree;
class Inspector;
class LogWidget;
class EnginePreview;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newScene();
    void openScene();
    void saveScene();
    void about();
    void setPlaying(bool playing);          ///< 播放/停止（▶/⏹）
    void pickSceneAt(float x, float y);     ///< 场景视图点击拾取

private:
    void createDocks();
    void createMenus();
    void createToolbar();

    Ui::MainWindow *ui;

    Viewport *m_sceneViewport;
    Viewport *m_gameViewport;
    SceneTree *m_sceneTree;
    Inspector *m_inspector;
    LogWidget *m_log;
    EnginePreview *m_scenePreview;
    EnginePreview *m_gamePreview;
    QAction *m_playAction;
};
#endif // MAINWINDOW_H
