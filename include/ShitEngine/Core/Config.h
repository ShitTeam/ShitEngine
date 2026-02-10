#pragma once
#include <nlohmann/json.hpp>
#include "Log.h"
#include "pch.h"
#include "Core.h"

namespace Shit {
	using Json = nlohmann::json;

	struct ProjectConfig // 项目配置
	{
		std::string name = "Example";
	};

	struct WindowConfig // 窗口配置
	{
		std::string title = "Example"; // 窗口标题
		unsigned int width = 1280;   // 宽
		unsigned int height = 720;  // 高
		unsigned int framerateLimit = 144; // 帧率限制
	};

	/**
	 * @brief 配置类
	 */
	class SHIT_API Config {
	public:
		// --- 成员方法 ---
		void init();
		void loadFromJson(Json& j);

		// --- 静态API ---
		static Config& GetInstance();
		inline static void Init() { GetInstance().init(); }
		inline static const ProjectConfig& GetProjectConfig() { return GetInstance().m_projectConfig; }
		inline static const WindowConfig& GetWindowConfig() { return GetInstance().m_windowConfig; }

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