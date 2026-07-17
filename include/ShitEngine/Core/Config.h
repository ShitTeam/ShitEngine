#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Log.h"
#include "Core.h"

namespace Shit {
	using Json = nlohmann::json;

	/** @brief 项目配置 */
	struct ProjectConfig {
		std::string name = "Example"; ///< 项目名称
	};

	/** @brief 窗口配置 */
	struct WindowConfig {
		std::string title = "Example";     ///< 窗口标题
		unsigned int width = 1280;         ///< 逻辑分辨率宽度
		unsigned int height = 720;         ///< 逻辑分辨率高度
		unsigned int targetFPS = 144;      ///< 帧率上限
	};

	/**
	 * @brief 配置管理类
	 *
	 * 从 settings.json 读取配置，若无配置文件则全字段使用默认值。
	 * 在 Game::Init() 中自动调用 Init()。
	 */
	class SHIT_API Config {
	public:
		bool init();                         ///< 加载 settings.json
		void loadFromJson(const Json& j);    ///< 从 JSON 对象加载配置

		// --- 静态API ---
		static Config& GetInstance();
		inline static bool Init() { return GetInstance().init(); }
		inline static const ProjectConfig& GetProjectConfig() { return GetInstance().m_projectConfig; } ///< 项目配置
		inline static const WindowConfig& GetWindowConfig() { return GetInstance().m_windowConfig; }   ///< 窗口配置

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;
		Config(Config&&) = delete;
		Config& operator=(Config&&) = delete;

	private:
		Config() = default;
		~Config() = default;

		ProjectConfig m_projectConfig;
		WindowConfig m_windowConfig;
	};
}