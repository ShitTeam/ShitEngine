#include "gameexporter.h"

#include "project.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <nlohmann/json.hpp>

namespace {

using json = nlohmann::json;

/// 复制单个文件到目标（自动建目录；目标已存在则先删后拷）
bool copyFile(const QString &src, const QString &dst, QString *err)
{
    if (!QFileInfo::exists(src)) {
        if (err) *err = QStringLiteral("源文件不存在：%1").arg(src);
        return false;
    }
    const QString dstAbs = QDir(dst).absolutePath();
    if (!QDir().mkpath(QFileInfo(dstAbs).absolutePath())) {
        if (err) *err = QStringLiteral("无法创建目录：%1").arg(QFileInfo(dstAbs).absolutePath());
        return false;
    }
    if (QFileInfo::exists(dstAbs)) QFile::remove(dstAbs);
    if (!QFile::copy(QFileInfo(src).absoluteFilePath(), dstAbs)) {
        if (err) *err = QStringLiteral("复制失败：%1 → %2").arg(src, dst);
        return false;
    }
    return true;
}

/// 递归复制目录（目标已存在时逐文件覆盖）
bool copyDirectory(const QString &srcDir, const QString &dstDir, QString *err)
{
    const QDir src(srcDir);
    if (!src.exists()) return true;   // 源目录不存在视为跳过

    if (!QDir().mkpath(dstDir)) {
        if (err) *err = QStringLiteral("无法创建目录：%1").arg(dstDir);
        return false;
    }
    for (const QFileInfo &info : src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (info.isDir()) {
            if (!copyDirectory(info.absoluteFilePath(), dstDir + "/" + info.fileName(), err))
                return false;
        } else {
            if (!copyFile(info.absoluteFilePath(), dstDir + "/" + info.fileName(), err))
                return false;
        }
    }
    return true;
}

/// SDK bin/ 中导出所需的运行库
struct SdkBinaries {
    QString runtimeExe;    ///< ShitRuntime.exe
    QString engineDll;     ///< ShitEngine.dll 或 ShitEngine-d.dll
    QStringList sdlDlls;   ///< 存在的 SDL3*.dll
};

/// 从 SDK bin/ 收集运行库；失败返回 false（err 说明缺失项）
bool collectSdkBinaries(const QString &sdkDir, SdkBinaries &out, QString *err)
{
    const QDir bin(sdkDir + "/bin");
    const QString runtime = bin.absoluteFilePath("ShitRuntime.exe");
    if (!QFileInfo::exists(runtime)) {
        if (err) *err = QStringLiteral("SDK 缺少 ShitRuntime.exe（请重新执行 cmake --install 更新 SDK）");
        return false;
    }
    out.runtimeExe = runtime;

    for (const QString &cand : { bin.absoluteFilePath("ShitEngine.dll"),
                                 bin.absoluteFilePath("ShitEngine-d.dll") }) {
        if (QFileInfo::exists(cand)) { out.engineDll = cand; break; }
    }
    if (out.engineDll.isEmpty()) {
        if (err) *err = QStringLiteral("SDK 缺少引擎 DLL（ShitEngine.dll / ShitEngine-d.dll）");
        return false;
    }

    for (const QString &name : { "SDL3.dll", "SDL3_image.dll", "SDL3_mixer.dll", "SDL3_ttf.dll" }) {
        const QString p = bin.absoluteFilePath(name);
        if (QFileInfo::exists(p)) out.sdlDlls << p;
    }
    return true;
}

/// 字符串字段的路径归类：不动 / 绝对路径且存在 / 项目根内相对路径且存在
enum class Resolved { NotResource, AbsoluteExisting, RelativeExisting };

Resolved classifyPath(const QString &value, const QString &projectRoot)
{
    if (value.isEmpty() || value.contains('\n')) return Resolved::NotResource;
    if (QDir::isAbsolutePath(value))
        return QFileInfo::exists(value) ? Resolved::AbsoluteExisting : Resolved::NotResource;
    return QFileInfo::exists(QDir(projectRoot).absoluteFilePath(value))
        ? Resolved::RelativeExisting : Resolved::NotResource;
}

/// 深度改写 JSON 中的字符串字段（资源路径）：
///  - 项目根内存在的相对路径 → 复制到导出包同相对位置，字段保持原值（运行时相对 exe 目录命中）
///  - 存在的绝对路径 → 复制到导出包 Assets/（已存在则跳过，避免覆盖 Assets 同名字体的同名文件），
///    字段改写为 "Assets/<basename>"
/// 不存在的字符串（名称、任意文本）保持原样。conflicts 收集同名冲突提示。
void rewriteResourcePaths(json &node, const QString &projectRoot, const QString &outDir,
                          QStringList *conflicts)
{
    if (node.is_string()) {
        const QString value = QString::fromStdString(node.get<std::string>());
        switch (classifyPath(value, projectRoot)) {
            case Resolved::NotResource:
                return;
            case Resolved::AbsoluteExisting: {
                const QString dst = QStringLiteral("Assets/%1").arg(QFileInfo(value).fileName());
                const QString dstAbs = outDir + "/" + dst;
                const bool alreadyThere = QFileInfo::exists(dstAbs);
                if (alreadyThere) {
                    if (conflicts && QFileInfo(dstAbs).absoluteFilePath() != QFileInfo(value).absoluteFilePath())
                        conflicts->append(QStringLiteral("资源随 Assets 目录已入包，引用统一为 %1（原绝对路径 %2 不再使用）")
                                          .arg(dst, QFileInfo(value).absoluteFilePath()));
                } else {
                    QDir().mkpath(QFileInfo(dstAbs).absolutePath());
                    if (QFile::copy(QFileInfo(value).absoluteFilePath(), dstAbs) && conflicts)
                        conflicts->append(QStringLiteral("已随导出复制：%1").arg(QFileInfo(value).absoluteFilePath()));
                }
                node = dst.toStdString();
                return;
            }
            case Resolved::RelativeExisting: {
                const QString cleaned = QDir::cleanPath(value);
                const QString abs = QDir(projectRoot).absoluteFilePath(value);
                if (cleaned == ".." || cleaned.startsWith("../")) {
                    // 相对路径逃逸项目根（如 ../xxx.png）：按绝对路径分支处理——
                    // 直接按原值复制会写进"导出包之外"（且运行时相对 exe 目录同样落空），
                    // 收进包内 Assets/ 并改写字段，保证产物自包含
                    const QString src = QFileInfo(abs).absoluteFilePath();
                    const QString dst = QStringLiteral("Assets/%1").arg(QFileInfo(src).fileName());
                    const QString dstAbs = outDir + "/" + dst;
                    if (!QFileInfo::exists(dstAbs)) {
                        QDir().mkpath(QFileInfo(dstAbs).absolutePath());
                        QFile::copy(src, dstAbs);
                    }
                    node = dst.toStdString();
                    return;
                }
                const QString dstAbs = outDir + "/" + value;
                if (QFileInfo(abs).isFile() && !QFileInfo::exists(dstAbs)) {
                    QDir().mkpath(QFileInfo(dstAbs).absolutePath());
                    QFile::copy(abs, dstAbs);
                }
                return;   // 保持原样
            }
        }
        return;
    }
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it)
            rewriteResourcePaths(it.value(), projectRoot, outDir, conflicts);
    } else if (node.is_array()) {
        for (auto &item : node)
            rewriteResourcePaths(item, projectRoot, outDir, conflicts);
    }
}

} // namespace

bool exportGame(const Project &project, const GameExportOptions &options,
                const std::function<void(const QString &)> &log, QString *err)
{
    const auto info = [&log](const QString &msg) { if (log) log(msg); };

    // 1. 前置校验
    const QString sdkDir = project.sdkDir();
    if (sdkDir.trimmed().isEmpty() || !QDir(sdkDir).exists()) {
        if (err) *err = QStringLiteral("项目未配置有效的引擎 SDK 目录（文件 → 项目设置… → 通用 → 引擎 SDK 目录）。");
        return false;
    }
    if (options.outDir.trimmed().isEmpty()) {
        if (err) *err = QStringLiteral("未选择输出目录。");
        return false;
    }
    if (options.gameName.trimmed().isEmpty()) {
        if (err) *err = QStringLiteral("未填写游戏名。");
        return false;
    }
    if (!QFileInfo::exists(options.scenePath)) {
        if (err) *err = QStringLiteral("起始场景不存在：%1").arg(options.scenePath);
        return false;
    }

    const QString outDir = QDir::cleanPath(options.outDir);
    const QString gameName = options.gameName.trimmed();

    // 2. 从 SDK 收集运行库
    SdkBinaries sdk;
    {
        QString sdkErr;
        if (!collectSdkBinaries(sdkDir, sdk, &sdkErr)) {
            if (err) *err = sdkErr;
            return false;
        }
    }

    if (!QDir().mkpath(outDir)) {
        if (err) *err = QStringLiteral("无法创建输出目录：%1").arg(outDir);
        return false;
    }

    info(QStringLiteral("── 导出游戏：%1 → %2 ──").arg(gameName, outDir));

    // 3. 运行库：exe（改名）+ 引擎 DLL + SDL 全家
    {
        const QString exeName = gameName + QStringLiteral(".exe");
        if (!copyFile(sdk.runtimeExe, outDir + "/" + exeName, err)) return false;
        info(QStringLiteral("✓ %1（运行时 ShitRuntime.exe）").arg(exeName));

        if (!copyFile(sdk.engineDll, outDir + "/" + QFileInfo(sdk.engineDll).fileName(), err)) return false;
        info(QStringLiteral("✓ %1").arg(QFileInfo(sdk.engineDll).fileName()));
        for (const QString &dll : sdk.sdlDlls) {
            if (!copyFile(dll, outDir + "/" + QFileInfo(dll).fileName(), err)) return false;
            info(QStringLiteral("✓ %1").arg(QFileInfo(dll).fileName()));
        }
        if (sdk.sdlDlls.size() < 4) {
            // P33：SDK 运行库不全 → 直接失败（此前仅 ⚠ 提示却返回成功，
            // 产出的包运行时会缺子系统崩溃，用户拿到"导出完成"的假象）
            if (err)
                *err = QStringLiteral("SDK 缺少部分 SDL 动态库（%1/4）——请重新安装完整 SDK（cmake --install）").arg(sdk.sdlDlls.size());
            return false;
        }
    }

    // 4. 项目脚本 DLL（未构建/无脚本工程时跳过）
    const QString pluginDll = project.pluginDllPath();
    if (!pluginDll.isEmpty() && QFileInfo::exists(pluginDll)) {
        if (!copyFile(pluginDll, outDir + "/" + QFileInfo(pluginDll).fileName(), err)) return false;
        info(QStringLiteral("✓ %1（游戏脚本）").arg(QFileInfo(pluginDll).fileName()));
    } else if (!pluginDll.isEmpty()) {
        // P33：项目配了脚本插件但未构建 → 直接失败（导出可运行的游戏包必须带脚本 DLL）
        if (err)
            *err = QStringLiteral("插件 DLL 未构建（%1）——请在编辑器按 Ctrl+B 构建脚本后再导出").arg(pluginDll);
        return false;
    }

    // 5. Assets 整目录 + 场景路径改写
    if (QDir(project.assetsDir()).exists()) {
        if (!copyDirectory(project.assetsDir(), outDir + "/Assets", err)) return false;
        info(QStringLiteral("✓ Assets/（%1）").arg(project.assetsDir()));
    } else {
        info(QStringLiteral("⚠ 项目无 Assets/ 目录，跳过"));
    }

    json scene;
    {
        QFile f(options.scenePath);
        if (!f.open(QIODevice::ReadOnly)) {
            if (err) *err = QStringLiteral("无法读取场景：%1").arg(options.scenePath);
            return false;
        }
        try {
            scene = json::parse(f.readAll().toStdString());
        } catch (const std::exception &e) {
            if (err) *err = QStringLiteral("场景解析失败：%1（%2）").arg(options.scenePath, e.what());
            return false;
        }
    }

    QStringList conflicts;
    rewriteResourcePaths(scene, project.rootDir(), outDir, &conflicts);
    for (const QString &c : conflicts) info(QStringLiteral("⚠ %1").arg(c));

    const QString sceneRel = QStringLiteral("Scenes/%1").arg(QFileInfo(options.scenePath).fileName());
    {
        QDir().mkpath(QFileInfo(outDir + "/" + sceneRel).absolutePath());
        QFile f(outDir + "/" + sceneRel);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err) *err = QStringLiteral("无法写入导出场景：%1").arg(outDir + "/" + sceneRel);
            return false;
        }
        f.write(scene.dump(2).c_str());
        f.flush();
    }
    info(QStringLiteral("✓ %1（资源路径已改写为导出包内相对路径）").arg(sceneRel));

    // 7. config.json：scene + plugins + inputMappings（引擎 Config 会合并读取 inputMappings）
    json cfg;
    cfg["scene"] = sceneRel.toStdString();
    if (!pluginDll.isEmpty() && QFileInfo::exists(pluginDll)) {
        cfg["plugins"] = json::array();
        cfg["plugins"].push_back({
            { "name", (project.name() + "Scripts").toStdString() },
            { "path", QFileInfo(pluginDll).fileName().toStdString() },
        });
    }
    if (project.inputMappings().is_object() && !project.inputMappings().empty())
        cfg["inputMappings"] = project.inputMappings();
    {
        QDir().mkpath(QFileInfo(outDir + "/config.json").absolutePath());
        QFile f(outDir + "/config.json");
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err) *err = QStringLiteral("无法写入导出配置：%1").arg(outDir + "/config.json");
            return false;
        }
        f.write(cfg.dump(2).c_str());
        f.flush();
    }
    info(QStringLiteral("✓ config.json（scene=%1）").arg(sceneRel));

    info(QStringLiteral("── 导出完成：%1 ——双击 %2 即可运行（已内置 chdir 到自身目录）──")
         .arg(gameName, gameName + ".exe"));
    return true;
}