#include "assetpaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {
QString g_projectRoot;
}

namespace AssetPaths {

void setProjectRoot(const QString& root)
{
    g_projectRoot = root;
}

const QString& projectRoot()
{
    return g_projectRoot;
}

QString toRelative(const QString& path)
{
    if (path.isEmpty()) return path;
    if (g_projectRoot.isEmpty()) return path;   // 无项目：保持原样（编辑器启动态存绝对）
    const QString abs = toAbsolute(path);
    const QString rel = QDir(g_projectRoot).relativeFilePath(abs);
    // 逃逸项目（../..）不具可移植性，存绝对
    if (rel.startsWith("../") || rel == ".." || rel == ".") return abs;
    return rel;
}

QString toAbsolute(const QString& path)
{
    if (path.isEmpty() || QDir::isAbsolutePath(path)) return path;
    if (!g_projectRoot.isEmpty()) {
        const QString inProject = QDir::cleanPath(QDir(g_projectRoot).absoluteFilePath(path));
        if (QFileInfo::exists(inProject)) return inProject;
    }
    // exe 目录及其旁常见资源根兜底（引擎运行时资源就在这些位置）
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString exeBased = QDir::cleanPath(QDir(appDir).absoluteFilePath(path));
    if (QFileInfo::exists(exeBased)) return exeBased;
    for (const QString& root : { "resource", "assets", "Assets" }) {
        const QString candidate = QDir::cleanPath(QDir(appDir + "/" + root).absoluteFilePath(path));
        if (QFileInfo::exists(candidate)) return candidate;
    }
    // 都不存在：项目根拼接返回（路径可见，引擎端报加载失败便于排查）
    if (!g_projectRoot.isEmpty())
        return QDir::cleanPath(QDir(g_projectRoot).absoluteFilePath(path));
    return path;
}

} // namespace AssetPaths
