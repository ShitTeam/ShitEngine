#include "project.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {

/// 读 qrc 资源为文本；失败返回空串
QString readText(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream ts(&f);
    return ts.readAll();
}

/// 模板占位替换：{{KEY}} → value。占位用 {{ }} 避开 CMake 的 ${ } 语法冲突。
QString renderTemplate(const QString &text,
                        std::initializer_list<std::pair<const char *, QString>> values)
{
    QString out = text;
    for (const auto &kv : values)
        out.replace(QStringLiteral("{{%1}}").arg(QLatin1String(kv.first)), kv.second);
    return out;
}

/// 写入文件（不存在则创建，已存在则截断）
bool writeFile(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    f.write(content.toUtf8());
    f.flush();
    return true;
}

/// 依序确保目录存在
bool ensureDirs(const QStringList &dirs)
{
    for (const QString &d : dirs)
        if (!QDir().mkpath(d)) return false;
    return true;
}

constexpr int kProjectConfigVersion = 1;

} // namespace

bool Project::open(const QString &rootDir)
{
    m_valid = false;
    m_rootDir = QDir(rootDir).absolutePath();

    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("项目根目录下没有 config.json: %1").arg(m_rootDir));
        return false;
    }

    try {
        m_config = nlohmann::json::parse(f.readAll().toStdString());
    } catch (const std::exception &e) {
        setError(QStringLiteral("config.json 解析失败: %1").arg(QString::fromUtf8(e.what())));
        return false;
    }

    if (!m_config->is_object() || !m_config->contains("name")) {
        setError(QStringLiteral("config.json 结构无效（缺少 name 字段）"));
        return false;
    }

    m_valid = true;
    return true;
}

bool Project::create(const QString &rootDir, const QString &name,
                     const QString &sdkDir, bool withScripts)
{
    m_valid = false;
    m_rootDir = QDir(rootDir).absolutePath();
    const QString projectName = name.trimmed();
    if (projectName.isEmpty()) {
        setError(QStringLiteral("项目名为空"));
        return false;
    }

    const QStringList dirs = {
        m_rootDir,
        m_rootDir + "/Scenes",
        m_rootDir + "/Assets",
        m_rootDir + "/.shitengine",
        m_rootDir + "/bin",
        m_rootDir + "/build",
    };
    QStringList withScriptDirs = dirs;
    if (withScripts) {
        withScriptDirs << m_rootDir + "/Scripts/src"
                       << m_rootDir + "/Scripts/generated/reflection";
    }
    if (!ensureDirs(withScripts ? withScriptDirs : dirs)) {
        setError(QStringLiteral("无法创建项目目录: %1").arg(m_rootDir));
        return false;
    }

    // ── config.json ──
    nlohmann::json doc;
    doc["name"] = projectName.toStdString();
    doc["version"] = kProjectConfigVersion;
    doc["engine"]["sdkDir"] = sdkDir.trimmed().toStdString();
    // 默认输入映射（P15：项目设置页可改；让新项目开箱即用动作/轴 API）
    doc["inputMappings"]["actions"]["Jump"] = { "Space" };
    doc["inputMappings"]["actions"]["Sprint"] = { "Left Shift" };
    doc["inputMappings"]["axes"]["Horizontal"] = { { "negative", { "A" } }, { "positive", { "D" } } };
    doc["inputMappings"]["axes"]["Vertical"] = { { "negative", { "S" } }, { "positive", { "W" } } };
    if (withScripts) {
        const std::string dllName = scriptTargetName(projectName).toStdString();
        doc["plugins"] = nlohmann::json::array();
        doc["plugins"].push_back({
            { "name", (projectName + "Scripts").toStdString() },
            { "path", "bin/" + dllName + ".dll" },
        });
    }
    m_config = std::move(doc);
    if (!writeConfigFile()) {
        setError(QStringLiteral("无法写入 %1").arg(configPath()));
        return false;
    }

    // ── .gitignore ──
    writeFile(m_rootDir + "/.gitignore",
              QStringLiteral("build/\nbin/\n.shitengine/\n"));

    // ── C++ 脚本工程（模板渲染，资源见 Editor/templates/）──
    if (withScripts) {
        const QString dllName = scriptTargetName(projectName);
        const QString sdk = sdkDir.trimmed();

        // 根 CMakeLists.txt（P16）：仅 IDE 便利——CLion / Visual Studio 打开项目根目录
        // 即得到完整 CMake 工程（编译入口仍是 Scripts/，现有构建流程不受影响）
        const QString rootCmake = renderTemplate(
            QStringLiteral("cmake_minimum_required(VERSION 3.16)\n"
                           "project(\"{{PROJECT_NAME}}\" LANGUAGES CXX)\n\n"
                           "add_subdirectory(Scripts)\n"),
            { { "PROJECT_NAME", projectName } });

        const QString cmake = renderTemplate(
            readText(QStringLiteral(":/templates/scripts/CMakeLists.txt.in")),
            { { "PROJECT_NAME", projectName },
              { "SCRIPT_DLL_NAME", dllName },
              { "SDK_DIR", sdk } });
        const QString exportSrc = renderTemplate(
            readText(QStringLiteral(":/templates/scripts/plugin_export.cpp.in")),
            { { "PLUGIN_NAME", projectName },
              { "PLUGIN_VERSION", "0.1.0" } });
        const QString behaviors = readText(QStringLiteral(":/templates/scripts/Behaviors.h.in"));

        const QString srcDir = m_rootDir + "/Scripts/src";
        if (!writeFile(m_rootDir + "/CMakeLists.txt", rootCmake)
            || !writeFile(m_rootDir + "/Scripts/CMakeLists.txt", cmake)
            || !writeFile(srcDir + "/plugin_export.cpp", exportSrc)
            || !writeFile(srcDir + "/Behaviors.h", behaviors)) {
            setError(QStringLiteral("无法生成脚本工程模板文件"));
            return false;
        }
    }

    m_valid = true;
    return true;
}

bool Project::saveConfig() const
{
    return writeConfigFile();
}

// ── 路径 ────────────────────────────────────────────────

QString Project::configPath() const { return m_rootDir + "/config.json"; }
QString Project::stateDir() const { return m_rootDir + "/.shitengine"; }
QString Project::stateFilePath() const { return m_rootDir + "/.shitengine/state.ini"; }
QString Project::assetsDir() const { return m_rootDir + "/Assets"; }
QString Project::scriptsDir() const { return m_rootDir + "/Scripts"; }
QString Project::buildDir() const { return m_rootDir + "/build/scripts"; }
QString Project::buildOutDir() const { return m_rootDir + "/build/out"; }
QString Project::binDir() const { return m_rootDir + "/bin"; }

// ── 配置 ────────────────────────────────────────────────

QString Project::name() const
{
    return m_config ? QString::fromStdString(m_config->value("name", "")) : QString();
}

QString Project::sdkDir() const
{
    if (!m_config) return QString();
    const auto it = m_config->find("engine");
    if (it == m_config->end() || !it->is_object()) return QString();
    return QString::fromStdString(it->value("sdkDir", ""));
}

void Project::setSdkDir(const QString &dir)
{
    if (!m_config) return;
    (*m_config)["engine"]["sdkDir"] = dir.trimmed().toStdString();
}

QString Project::scenePath() const
{
    if (!m_config) return QString();
    const auto it = m_config->find("scene");
    if (it == m_config->end() || !it->is_string()) return QString();
    const std::string rel = it->get<std::string>();
    if (rel.empty()) return QString();
    return QDir::cleanPath(QDir(m_rootDir).absoluteFilePath(QString::fromStdString(rel)));
}

void Project::setScenePath(const QString &absolutePath)
{
    if (!m_config) return;
    const QString path = QDir::cleanPath(absolutePath);
    if (path.isEmpty()) {           // 空 = 未配置启动场景 → 移除字段
        m_config->erase("scene");
        return;
    }
    const QDir root(m_rootDir);
    const QString rel = root.relativeFilePath(path);
    if (rel.isEmpty() || rel == "." || rel.startsWith("..")) {
        m_config->erase("scene");   // 项目根之外不记录
        return;
    }
    (*m_config)["scene"] = rel.toStdString();
}

nlohmann::json Project::inputMappings() const
{
    if (!m_config) return nlohmann::json::object();
    const auto it = m_config->find("inputMappings");
    if (it == m_config->end() || !it->is_object()) return nlohmann::json::object();
    return *it;
}

void Project::setInputMappings(const nlohmann::json &mappings)
{
    if (!m_config) return;
    if (mappings.is_object() && !mappings.empty()) {
        (*m_config)["inputMappings"] = mappings;
    } else {
        m_config->erase("inputMappings");
    }
}

QString Project::ideExePath() const
{
    if (!m_config) return QString();
    const auto it = m_config->find("editor");
    if (it == m_config->end() || !it->is_object()) return QString();
    return QString::fromStdString(it->value("ideExe", ""));
}

void Project::setIdeExePath(const QString &path)
{
    if (!m_config) return;
    const QString p = QDir::cleanPath(path.trimmed());
    if (p.isEmpty()) {
        auto it = m_config->find("editor");
        if (it != m_config->end() && it->is_object()) {
            it->erase("ideExe");
            if (it->empty()) m_config->erase("editor");
        }
        return;
    }
    (*m_config)["editor"]["ideExe"] = QDir::toNativeSeparators(p).toStdString();
}

QString Project::pluginDllPath() const
{
    if (!m_config) return QString();
    const auto it = m_config->find("plugins");
    if (it == m_config->end() || !it->is_array() || it->empty()) return QString();
    const std::string rel = it->front().value("path", "");
    if (rel.empty()) return QString();
    return QDir::cleanPath(QDir(m_rootDir).absoluteFilePath(QString::fromStdString(rel)));
}

QString Project::pluginDllFileName() const
{
    const QString p = pluginDllPath();
    return p.isEmpty() ? QString() : QFileInfo(p).fileName();
}

QString Project::scriptTargetName() const
{
    return scriptTargetName(name());
}

QString Project::scriptTargetName(const QString &projectName)
{
    return projectName + "Scripts";
}

bool Project::hasScripts() const
{
    return QFileInfo::exists(m_rootDir + "/Scripts/CMakeLists.txt");
}

// ── 内部 ────────────────────────────────────────────────

bool Project::writeConfigFile() const
{
    if (!m_config) return false;
    return writeFile(configPath(), QString::fromStdString(m_config->dump(2)));
}

void Project::setError(const QString &msg)
{
    m_error = msg;
}