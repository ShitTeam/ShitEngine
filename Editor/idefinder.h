#pragma once

#include <QString>
#include <QStringList>

/// 一个可用 IDE 的探测结果（名称 + 可执行文件绝对路径）
struct IdeInfo {
    QString name;        ///< 显示名（如 "Visual Studio Code"）
    QString executable;  ///< 可执行文件绝对路径（devenv / clion64 / qtcreator / Code.exe）
};

/// 探测本机已安装的 IDE（P16）：
/// 依次尝试 PATH 与常见安装路径；Visual Studio 走 vswhere 查询，失败回退枚举
/// VS2022 / VS18（Community/Professional/Enterprise/BuildTools）常见位置。
/// 未找到任何 IDE 时返回空列表（设置页下拉仅剩「浏览…」）。
QList<IdeInfo> detectInstalledIdes();

/// 启动 IDE 打开项目根目录（startDetached，不阻塞编辑器）。
/// 失败返回 false，err 携带原因（用于日志/弹窗）。
bool startIdeProject(const IdeInfo &ide, const QString &projectRoot, QString *err);