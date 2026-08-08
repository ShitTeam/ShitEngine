#include "scriptbuilder.h"

#include <QDir>
#include <QFileInfo>

ScriptBuilder::ScriptBuilder(QObject *parent)
    : QObject(parent)
{
}

ScriptBuilder::~ScriptBuilder()
{
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning)
            m_process->kill();
    }
}

ScriptBuilder::Toolchain ScriptBuilder::detectToolchain(const QString &sdkDir)
{
    QDir lib(sdkDir + "/lib");
    if (lib.exists()) {
        if (!lib.entryList({ QStringLiteral("ShitEngine*.lib") }, QDir::Files).isEmpty())
            return Toolchain::MSVC;
        if (!lib.entryList({ QStringLiteral("libShitEngine*.dll.a") }, QDir::Files).isEmpty())
            return Toolchain::MinGW;
    }
    return Toolchain::Unknown;
}

void ScriptBuilder::build(const QString &scriptsDir, const QString &buildDir,
                          const QString &sdkDir, const QString &binDir)
{
    if (m_process) {
        emit buildFailed(tr("已有构建进行中"));
        return;
    }
    m_scriptsDir = scriptsDir;
    m_buildDir = buildDir;
    m_sdkDir = sdkDir;
    m_binDir = binDir;
    m_configured = false;
    m_lastConfigureFailed = false;
    m_vsGeneratorIndex = 0;   // 每次构建从首选生成器开始

    if (!QFile::exists(scriptsDir + "/CMakeLists.txt")) {
        emit buildFailed(tr("脚本工程不存在：%1/CMakeLists.txt").arg(scriptsDir));
        return;
    }
    if (!QDir(sdkDir).exists()) {
        emit buildFailed(tr("引擎 SDK 目录无效：%1\n请通过「项目 → 项目设置…」配置 SDK。").arg(sdkDir));
        return;
    }
    if (detectToolchain(sdkDir) == Toolchain::Unknown) {
        emit buildFailed(tr("无法从 SDK 识别编译器（lib 目录下既无 ShitEngine*.lib 也无 libShitEngine*.dll.a）：%1").arg(sdkDir));
        return;
    }

    QDir().mkpath(buildDir);
    startConfigure();
}

QStringList ScriptBuilder::makeConfigureArgs() const
{
    QString sdk = m_sdkDir;
    sdk.replace('\\', '/');

    QStringList args;
    args << QStringLiteral("-S") << m_scriptsDir
         << QStringLiteral("-B") << m_buildDir
         << QStringLiteral("-DSHITENGINE_SDK_DIR=%1").arg(sdk);

    const Toolchain tc = detectToolchain(m_sdkDir);
    if (tc == Toolchain::MSVC) {
        args << QStringLiteral("-G") << m_vsGenerators.at(m_vsGeneratorIndex)
             << QStringLiteral("-A") << QStringLiteral("x64");
    } else {
        args << QStringLiteral("-G") << QStringLiteral("Ninja")
             << QStringLiteral("-DCMAKE_C_COMPILER=gcc")
             << QStringLiteral("-DCMAKE_CXX_COMPILER=g++")
             << QStringLiteral("-DCMAKE_BUILD_TYPE=Debug");
    }
    return args;
}

QStringList ScriptBuilder::makeBuildArgs() const
{
    QStringList args{ QStringLiteral("--build"), m_buildDir };
    if (detectToolchain(m_sdkDir) == Toolchain::MSVC)
        args << QStringLiteral("--config") << QStringLiteral("Debug");
    return args;
}

void ScriptBuilder::startConfigure()
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const auto out = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        if (!out.trimmed().isEmpty()) emit buildOutput(out.trimmed());
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        const auto err = QString::fromLocal8Bit(m_process->readAllStandardError());
        if (!err.trimmed().isEmpty()) emit buildOutput(err.trimmed());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScriptBuilder::onProcessFinished);

    emit buildOutput(tr("── 配置脚本工程（%1）──").arg(QFileInfo(m_scriptsDir).fileName()));
    m_process->start(QStringLiteral("cmake"), makeConfigureArgs());
}

void ScriptBuilder::startBuild()
{
    // QProcess 复用：新进程对象
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const auto out = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        if (!out.trimmed().isEmpty()) emit buildOutput(out.trimmed());
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        const auto err = QString::fromLocal8Bit(m_process->readAllStandardError());
        if (!err.trimmed().isEmpty()) emit buildOutput(err.trimmed());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScriptBuilder::onProcessFinished);

    emit buildOutput(tr("-- 编译脚本工程 --"));
    m_process->start(QStringLiteral("cmake"), makeBuildArgs());
}

void ScriptBuilder::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess *p = m_process;
    const bool wasConfigure = !m_configured;
    m_process = nullptr;
    p->deleteLater();

    const bool ok = (status == QProcess::NormalExit && exitCode == 0);

    if (wasConfigure) {
        m_configured = ok;
        m_lastConfigureFailed = !ok;
        if (ok) {
            emit buildOutput(tr("-- 配置完成 --"));
            startBuild();
            return;
        }
        // MSVC 且还有 VS 生成器候选 → 切换重试（不同生成器的缓存需清空）
        if (m_vsGeneratorIndex + 1 < m_vsGenerators.size()) {
            ++m_vsGeneratorIndex;
            emit buildOutput(tr("-- VS 生成器 %1 不可用，改用 %2 --")
                .arg(m_vsGenerators.at(m_vsGeneratorIndex - 1), m_vsGenerators.at(m_vsGeneratorIndex)));
            QDir(m_buildDir).removeRecursively();
            startConfigure();
            return;
        }
    }
    if (!ok) {
        finishFailure(tr("cmake 退出码 %1%2").arg(exitCode)
            .arg(status == QProcess::CrashExit ? tr("（进程异常退出）") : QString()));
        return;
    }

    // build 成功 → 检查 DLL 是否已产出
    finishSuccess();
}

void ScriptBuilder::finishSuccess()
{
    if (m_binDir.isEmpty()) return; // defensive
    emit buildOutput(tr("-- 构建完成：产物目录 %1 --").arg(m_binDir));
    emit buildFinished(true);
}

void ScriptBuilder::finishFailure(const QString &reason)
{
    emit buildOutput(tr("** 构建失败：%1 **").arg(reason));
    emit buildFailed(reason);
}