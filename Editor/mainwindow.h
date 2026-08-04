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

private:
    void createDocks();
    void createMenus();

    Ui::MainWindow *ui;

    Viewport *m_viewport;
    SceneTree *m_sceneTree;
    Inspector *m_inspector;
    LogWidget *m_log;
    EnginePreview *m_preview;
};
#endif // MAINWINDOW_H
