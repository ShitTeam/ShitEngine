#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QStringList>

#include <nlohmann/json.hpp>

#include <optional>

/// P14 项目模型：一个项目 = 目录 + config.json（配置）+ .shitengine/（编辑器私有状态）。
///
/// 目录骨架：
///   <Root>/config.json          项目配置（name / engine.sdkDir / plugins / scene）
///   <Root>/Scenes/              场景目录（初始 Main.scene）
///   <Root>/Assets/              资源目录
///   <Root>/Scripts/             C++ 脚本工程（插件 DLL 源码）
///   <Root>/build/               CMake 缓存（gitignore）
///   <Root>/bin/                 DLL 产物（gitignore）
///   <Root>/.shitengine/state.ini 编辑器状态（dock/几何/最近场景，QSettings IniFormat）
///   <Root>/.gitignore
class Project
{
public:
    Project() = default;

    /// 打开已有项目目录：读 config.json 校验。成功返回 true。
    /// 根目录下无 config.json → 视为非法项目（error() 可查原因）。
    bool open(const QString &rootDir);

    /// 新建项目骨架：建目录 + 写 config.json/.gitignore/.shitengine 占位，返回 true。
    /// 不会创建场景文件（由编辑器用引擎序列化生成，保证格式正确）。
    bool create(const QString &rootDir, const QString &name,
                const QString &sdkDir, bool withScripts);

    /// 保存 config.json 回磁盘（如场景/插件路径变化后）
    bool saveConfig() const;

    bool isValid() const { return m_valid; }
    QString error() const { return m_error; }

    // ---- 路径 ----
    QString rootDir() const { return m_rootDir; }
    QString configPath() const;
    QString stateDir() const;      ///< .shitengine/
    QString stateFilePath() const; ///< .shitengine/state.ini
    QString assetsDir() const;     ///< Assets/
    QString scriptsDir() const;    ///< Scripts/
    QString buildDir() const;      ///< build/scripts（CMake 缓存）
    QString buildOutDir() const;   ///< build/out（构建产物临时目录，避开编辑器占用 bin/）
    QString binDir() const;        ///< bin/
    QString scenesDir() const { return rootDir() + "/Scenes"; }

    // ---- 配置 ----
    QString name() const;
    /// 引擎 SDK 绝对路径（构建脚本工程时 find_package 用）
    QString sdkDir() const;
    void setSdkDir(const QString &dir);
    /// 项目启动场景绝对路径（config.scene 相对根解析；未配置返回空）
    QString scenePath() const;
    /// 记录场景路径（config.scene 写相对项目根的路径，空则移除该字段）
    void setScenePath(const QString &absolutePath);
    /// 单个插件 DLL 绝对路径（config.plugins[].path 按项目根解析）
    QString pluginDllPath() const;
    /// 插件 DLL 基础文件名（不含路径，如 "MyProjectScripts.dll"；无插件返回空）
    QString pluginDllFileName() const;
    /// 脚本工程输出 DLL 名（不含扩展名，如 "MyProjectScripts"）
    QString scriptTargetName() const;
    /// 由项目名推导 DLL 名（静态，供文档/向导使用）
    static QString scriptTargetName(const QString &projectName);

    bool hasScripts() const;   ///< Scripts/CMakeLists.txt 是否已生成

    // ---- 输入映射（config.json 的 inputMappings 段，与引擎 settings.json 同构）----
    /// 输入映射配置；项目未配置时返回空对象
    nlohmann::json inputMappings() const;
    /// 写输入映射（空对象 → 移除该段）
    void setInputMappings(const nlohmann::json &mappings);

    // ---- 代码编辑器（config.json 的 editor.ideExe）----
    /// 项目配置的 IDE 可执行文件绝对路径（空 = 未配置，打开代码时引导去设置）
    QString ideExePath() const;
    void setIdeExePath(const QString &path);

private:
    bool writeConfigFile() const;
    void setError(const QString &msg);

    std::optional<nlohmann::json> m_config;
    QString m_rootDir;
    QString m_error;
    bool m_valid = false;
};
#endif