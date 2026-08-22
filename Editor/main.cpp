#include "mainwindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 命令行入口——`Editor.exe <xxx.scene>` 直接打开场景（向上查找所属项目则连带
    // 打开项目）；`Editor.exe --project <dir>` 打开项目。方便 .scene 文件关联与脚本调用。
    QString projectArg;
    QString sceneArg;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QStringLiteral("--project") && i + 1 < argc) {
            projectArg = QString::fromLocal8Bit(argv[++i]);
        } else if (arg.endsWith(QStringLiteral(".scene"), Qt::CaseInsensitive)) {
            sceneArg = arg;
        }
    }

    MainWindow w;
    w.show();

    // 排队到事件循环启动后执行（窗口与预览就绪再打开，避免构造期弹窗/嵌套事件）
    if (!projectArg.isEmpty() || !sceneArg.isEmpty()) {
        const QString proj = projectArg;
        const QString scene = sceneArg;
        QTimer::singleShot(0, &w, [&w, proj, scene] { w.openFromCommandLine(proj, scene); });
    }

    return a.exec();
}