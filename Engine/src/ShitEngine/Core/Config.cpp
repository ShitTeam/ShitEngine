#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Config.h"
#include <fstream>

namespace Shit {
	bool Config::init() {
		std::ifstream file("settings.json");
		if (!file.is_open()) {
			ST_CORE_WARN("配置文件无法打开，正在使用默认配置。");
			return true;
		}

		Json j;
		try {
			file >> j;
		} catch (const std::exception& e) {
			ST_CORE_WARN("配置文件解析失败: {}，使用默认配置", e.what());
			return true;
		}
		loadFromJson(j);
		return true;
	}

	void Config::loadFromJson(const Json& j) {
		if (j.contains("project")) {
			// "project": { "name": "..." } 或 "project": "My Game"（向后兼容）
			if (j["project"].is_object() && j["project"].contains("name")) {
				m_projectConfig.name = j["project"]["name"].get<std::string>();
			} else if (j["project"].is_string()) {
				m_projectConfig.name = j["project"].get<std::string>();
			}
		}
		if (j.contains("window")) {
			if (j["window"].contains("title")) {
				m_windowConfig.title = j["window"]["title"].get<std::string>();
			}
			if (j["window"].contains("width")) {
				unsigned int w = j["window"]["width"].get<unsigned int>();
				m_windowConfig.width = (w > 0) ? w : 800;
			}
			if (j["window"].contains("height")) {
				unsigned int h = j["window"]["height"].get<unsigned int>();
				m_windowConfig.height = (h > 0) ? h : 600;
			}
			if (j["window"].contains("targetFPS")) {
				m_windowConfig.targetFPS = j["window"]["targetFPS"].get<unsigned int>();
			}
		}
		if (j.contains("inputMappings")) {
			const auto& im = j["inputMappings"];

			// 加载动作绑定
			if (im.contains("actions")) {
				for (auto& [actionName, bindings] : im["actions"].items()) {
					ActionBinding ab;
					if (bindings.is_array()) {
						for (auto& key : bindings) {
							ab.keys.push_back(key.get<std::string>());
						}
					}
					m_inputMappings.actions[actionName] = ab;
				}
			}

			// 加载轴绑定
			if (im.contains("axes")) {
				for (auto& [axisName, cfg] : im["axes"].items()) {
					AxisBinding ab;
					if (cfg.contains("negative")) {
						for (auto& key : cfg["negative"]) {
							ab.negative.push_back(key.get<std::string>());
						}
					}
					if (cfg.contains("positive")) {
						for (auto& key : cfg["positive"]) {
							ab.positive.push_back(key.get<std::string>());
						}
					}
					m_inputMappings.axes[axisName] = ab;
				}
			}
		}
	}

	Config& Config::GetInstance() {
		static Config instance;
		return instance;
	}
} // namespace Shit