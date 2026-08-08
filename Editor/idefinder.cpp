#include "idefinder.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace {

/// 依次取第一个存在的文件；均不存在返回空
QString firstExisting(const QStringList &candidates)
{
    for (const QString &p : candidates)
        if (QFileInfo(p).isFile()) return QDir::cleanPath(p);
    return QString();
}

/// 枚举 dirTemplate（含一个 `*` 段的目录）下的子目录，返回首个
/// `<sub>/bin/<fileName>` 存在的完整路径。用于 JetBrains 版本目录等场景
/// （如 `...\CLion\*\bin\clion64.exe` 或 `...\CLion*\bin\clion64.exe`）。
QString findVersionedExe(const QString &dirTemplate, const QString &fileName)
{
    const int star = dirTemplate.indexOf('*');
    if (star < 0) return QString();
    const QString prefix = dirTemplate.left(star);
    const QString suffix = dirTemplate.mid(star + 1);
    const QStringList subs = QDir(prefix).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &sub : subs) {
        const QString candidate = prefix + sub + suffix + "/bin/" + fileName;
        if (QFileInfo::exists(candidate)) return QDir::cleanPath(candidate);
    }
    return QString();
}

/// Visual Studio：vswhere 查询最新 NativeDesktop 安装的 devenv.exe（同步 5s 超时）
QString findDevenvViaVswhere()
{
    const QString vswhere = QDir::cleanPath(
        qEnvironmentVariable("ProgramFiles(x86)") + "/Microsoft Visual Studio/Installer/vswhere.exe");
    if (!QFileInfo::exists(vswhere)) return QString();

    QProcess proc;
    proc.start(vswhere, { "-latest", "-requires", "Microsoft.VisualStudio.Workload.NativeDesktop",
                          "-find", "**\\Common7\\IDE\\devenv.exe" });
    if (!proc.waitForStarted(3000)) return QString();
    if (!proc.waitForFinished(5000)) { proc.kill(); return QString(); }
    const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    const QString line = out.split('\n').first().trimmed();
    return line.isEmpty() ? QString() : QDir::cleanPath(line);
}

/// 由 `code` 启动器（bin\code.exe / code.cmd / 无扩展名 shell 脚本）转到安装根的 Code.exe：
/// 非 exe 时把所在目录上跳一级（bin → 安装根）再拼 Code.exe。
QString vsCodeExeFromLauncher(const QString &launcher)
{
    const QFileInfo fi(launcher);
    if (fi.suffix().compare("exe", Qt::CaseInsensitive) == 0) return fi.absoluteFilePath();
    QDir d = fi.absoluteDir();          // bin
    d.cdUp();                           // 安装根（Code.exe 与 bin 同级）
    const QString alt = d.absolutePath() + "/Code.exe";
    return QFileInfo::exists(alt) ? QDir::cleanPath(alt) : QString();
}

QString findVSCode()
{
    const QString local = qEnvironmentVariable("LOCALAPPDATA");
    const QString pf = qEnvironmentVariable("ProgramFiles");
    const QString pfx = qEnvironmentVariable("ProgramFiles(x86)");
    const QStringList installed = {
        local + "/Programs/Microsoft VS Code/Code.exe",
        pf + "/Microsoft VS Code/Code.exe",
        pfx + "/Microsoft VS Code/Code.exe",
    };

    // 1) Windows SearchPath 语义（PATH + 注册表 App Paths，后者由 VS Code 安装器写入）
    const QString found = QStandardPaths::findExecutable("code");
    if (!found.isEmpty()) {
        const QString exe = vsCodeExeFromLauncher(found);
        if (!exe.isEmpty()) return exe;
    }

    // 2) 常见安装位置
    const QString exe2 = firstExisting(installed);
    if (!exe2.isEmpty()) return exe2;

    // 3) PATH 手动扫描兜底（findExecutable 只认可执行扩展名，找不到无扩展名的 `code` 脚本）
    const QString pathEnv = qEnvironmentVariable("PATH");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QStringList pathDirs = pathEnv.split(';', Qt::SkipEmptyParts);
#else
    const QStringList pathDirs = pathEnv.split(';', QString::SkipEmptyParts);
#endif
    for (const QString &dir : pathDirs) {
        const QString candidate = QDir::cleanPath(dir + "/code");
        if (!QFileInfo::exists(candidate)) continue;
        const QString exe = vsCodeExeFromLauncher(candidate);
        if (!exe.isEmpty()) return exe;
    }
    return QString();
}

QString findDevenv()
{
    const QString exe = findDevenvViaVswhere();
    if (!exe.isEmpty()) return exe;

    const QString pf = qEnvironmentVariable("ProgramFiles");
    const QString pfx = qEnvironmentVariable("ProgramFiles(x86)");
    QStringList candidates;
    for (const QString &ver : { "2022", "18" })          // VS2022 与 VS18（2026）
        for (const QString &ed : { "Community", "Professional", "Enterprise", "BuildTools" }) {
            candidates << pf + "/Microsoft Visual Studio/" + ver + "/" + ed + "/Common7/IDE/devenv.exe"
                       << pfx + "/Microsoft Visual Studio/" + ver + "/" + ed + "/Common7/IDE/devenv.exe";
        }
    return firstExisting(candidates);
}

QString findClion()
{
    const QString local = qEnvironmentVariable("LOCALAPPDATA");
    const QString pf = qEnvironmentVariable("ProgramFiles");
    const QString pfx = qEnvironmentVariable("ProgramFiles(x86)");

    // Toolbox 安装：%LOCALAPPDATA%\JetBrains\Toolbox\apps\CLion\*\bin\clion64.exe
    if (!local.isEmpty()) {
        const QString exe = findVersionedExe(local + "/JetBrains/Toolbox/apps/CLion/", "clion64.exe");
        if (!exe.isEmpty()) return exe;
    }
    // 传统安装：%ProgramFiles%\JetBrains\CLion*\bin\clion64.exe
    for (const QString &base : { pf, pfx }) {
        if (base.isEmpty()) continue;
        const QString exe = findVersionedExe(base + "/JetBrains/CLion", "clion64.exe");
        if (!exe.isEmpty()) return exe;
    }
    // 任意盘根目录安装（如 D:\CLion）：PATH 中 CLion 自带 MinGW 的 g++/gcc
    // （路径含 /clion.../bin/mingw/bin/）→ 上溯 3 级到 CLion 根 → bin\clion64.exe
    for (const char *tool : { "g++", "gcc" }) {
        const QString t = QStandardPaths::findExecutable(tool);
        if (t.isEmpty() || !t.contains("/clion", Qt::CaseInsensitive)) continue;
        QDir d = QFileInfo(t).absoluteDir();          // bin
        for (int i = 0; i < 3 && d.cdUp(); ++i) { /* mingw → bin → CLion 根 */ }
        const QString alt = d.absolutePath() + "/bin/clion64.exe";
        if (QFileInfo::exists(alt)) return QDir::cleanPath(alt);
    }
    return QString();
}

QString findQtCreator()
{
    QString exe = QStandardPaths::findExecutable("qtcreator");
    if (!exe.isEmpty()) return QDir::cleanPath(exe);

    // Qt 安装树根目录：C:\Qt（默认）→ 或 qmake 在 PATH 时反查（Qt/<ver>/<abi>/bin 上溯 3 层 → Tools/QtCreator）
    const QString qtRoot = qEnvironmentVariable("QTDIR");
    if (!qtRoot.isEmpty()) {
        exe = firstExisting({ qtRoot + "/Tools/QtCreator/bin/qtcreator.exe" });
        if (!exe.isEmpty()) return exe;
    }
    const QString qmake = QStandardPaths::findExecutable("qmake");
    if (!qmake.isEmpty()) {
        QDir d = QFileInfo(qmake).absoluteDir();   // <abi>/bin
        for (int i = 0; i < 3 && d.cdUp(); ++i) { /* Qt → <ver> → 安装根 */ }
        const QString alt = d.absolutePath() + "/Tools/QtCreator/bin/qtcreator.exe";
        if (QFileInfo::exists(alt)) return QDir::cleanPath(alt);
    }
    return firstExisting({ "C:/Qt/Tools/QtCreator/bin/qtcreator.exe" });
}

} // namespace

QList<IdeInfo> detectInstalledIdes()
{
    QList<IdeInfo> out;
    const auto add = [&out](const QString &name, const QString &exe) {
        if (exe.isEmpty()) return;
        out.append({ name, QDir::cleanPath(exe) });
    };
    add(QStringLiteral("Visual Studio Code"), findVSCode());
    add(QStringLiteral("Visual Studio (devenv)"), findDevenv());
    add(QStringLiteral("CLion"), findClion());
    add(QStringLiteral("Qt Creator"), findQtCreator());
    return out;
}

bool startIdeProject(const IdeInfo &ide, const QString &projectRoot, QString *err)
{
    const QStringList args{ QDir::toNativeSeparators(projectRoot) };
    if (!QProcess::startDetached(ide.executable, args)) {
        if (err)
            *err = QStringLiteral("无法启动 IDE %1：%2").arg(ide.name, ide.executable);
        return false;
    }
    return true;
}