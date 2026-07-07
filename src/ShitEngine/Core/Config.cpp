#include "ShitEngine/Core/Config.h"
#include <fstream>

namespace Shit {
	bool Config::init() // 初始化
	{
		std::ifstream file("settings.json");
		if (!file.is_open()) {
			ST_CORE_WARN("配置文件无法打开，正在使用默认配置。");
			return true; // 使用默认值，不阻止引擎初始化
		}

		Json j;
		file >> j;
		loadFromJson(j);
		return true;
	}

	void Config::loadFromJson(const Json& j) // 读取Json配置
	{
		if (j.contains("project")) {
			m_projectConfig.name = j["project"].get<std::string>();
		}
		if (j.contains("window")) {
			if (j["window"].contains("title")) {
				m_windowConfig.title = j["window"]["title"].get<std::string>();
			}
			if (j["window"].contains("width")) {
				m_windowConfig.width = j["window"]["width"].get<unsigned int>();
			}
			if (j["window"].contains("height")) {
				m_windowConfig.height = j["window"]["height"].get<unsigned int>();
			}
			if (j["window"].contains("targetFPS")) {
				m_windowConfig.targetFPS = j["window"]["targetFPS"].get<unsigned int>();
			}
		}
	}

	Config& Config::GetInstance() { // 获取单例
		static Config instance;
		return instance;
	}
}