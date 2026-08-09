#pragma once

#include <QString>
#include <QStringList>
#include <functional>

class Project;

/// P18 导出游戏：把项目装配为可独立运行的绿色游戏包。
/// 包内 exe 已 chdir 到自身目录（Runtime 硬化），任意位置双击可运行。
struct GameExportOptions
{
    QString outDir;      ///< 输出目录（不存在则创建；内容将被组装）
    QString gameName;    ///< 游戏可执行名（如 "MyGame" → MyGame.exe）
    QString scenePath;   ///< 要导出的场景绝对路径（项目 Scenes/ 内）
};

/// 执行导出。log 逐行回调（进度/错误，可接日志面板或对话框文本区）。
/// 返回 true 表示成功；失败返回 false 并在 err 写原因（部分文件缺失类成功会 WARN 而非失败）。
bool exportGame(const Project &project, const GameExportOptions &options,
                const std::function<void(const QString &)> &log, QString *err);