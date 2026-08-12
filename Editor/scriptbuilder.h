#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class QProcess;

/// P14 脚本工程编译器：封装 cmake configure + build（异步 QProcess 状态机）。
///
/// 用法：
///   builder->build(scriptsDir, buildDir, sdkDir, binDir);
/// 流程：configure（未配置或 SDK 变化）→ build → 检查 DLL 输出 → buildFinished。
/// 编译器随 SDK 自动探测：SDK 由 MSVC 构建 → VS 生成器；MinGW → Ninja+gcc/g++。
class ScriptBuilder : public QObject
{
    Q_OBJECT
public:
    explicit ScriptBuilder(QObject *parent = nullptr);
    ~ScriptBuilder();

    /// 是否正在构建（避免并发触发）
    bool isBuilding() const { return m_process != nullptr; }

    /// 启动一次构建（异步）。sdkDir 为空/不存在时失败并发出 buildFailed。
    void build(const QString &scriptsDir, const QString &buildDir,
               const QString &sdkDir, const QString &binDir);

    /// 探测引擎 SDK 编译器族（lib 目录下的导入库形态）
    enum class Toolchain { Unknown, MSVC, MinGW };
    static Toolchain detectToolchain(const QString &sdkDir);

signals:
    /// 构建整体结束（success=重编译成功且 DLL 已产出/更新）
    void buildFinished(bool success);
    /// 构建提前失败（配置缺失/不可达），reason 供日志/弹窗
    void buildFailed(const QString &reason);
    /// cmake 原始输出（含进度与错误信息），供日志面板
    void buildOutput(const QString &line);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void startConfigure();
    void startBuild();
    QStringList makeConfigureArgs() const;
    QStringList makeBuildArgs() const;
    /// 按 SDK bin/ 的引擎 DLL 形态选择插件构建配置：
    /// 只有 ShitEngine-d.dll → Debug；否则（仅 Release 或两者皆有）→ Release。
    /// Debug 插件与 Release 引擎 DLL 混用会因 _ITERATOR_DEBUG_LEVEL 不一致，
    /// 在 RegisterPluginTypes 跨 DLL 传 std::string 时崩溃。
    QString sdkBuildConfig() const;
    void finishSuccess();
    void finishFailure(const QString &reason);

    QProcess *m_process = nullptr;
    QString m_scriptsDir;
    QString m_buildDir;
    QString m_sdkDir;
    QString m_binDir;
    bool m_configured = false;   ///< 已成功 configure（本次会话内）
    bool m_lastConfigureFailed = false;  ///< configure 尝试过并失败（避免死循环）
    /// MSVC SDK 下的 VS 生成器候选（依次尝试，失败切换并清缓存重配）
    QStringList m_vsGenerators{ QStringLiteral("Visual Studio 17 2022"),
                                QStringLiteral("Visual Studio 18 2026") };
    int m_vsGeneratorIndex = 0;
};

#endif // SCRIPTBUILDER_H