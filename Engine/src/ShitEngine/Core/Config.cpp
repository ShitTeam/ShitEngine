#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Core/Config.h"
#include <fstream>

namespace Shit {
	bool Config::init() {
		Json j;
		{
			std::ifstream file("settings.json");
			if (!file.is_open()) {
				ST_CORE_WARN("配置文件无法打开，正在使用默认配置。");
			} else {
				try {
					file >> j;
				} catch (const std::exception& e) {
					ST_CORE_WARN("配置文件解析失败: {}，使用默认配置", e.what());
				}
			}
		}

		// 项目级 config.json 的 inputMappings 覆盖 settings.json（编辑器「项目设置」页写入，
		// 使独立 Runtime 与编辑器播放共用同一套按键映射；无 config.json 时行为不变）
		{
			std::ifstream cfg("config.json");
			if (cfg.is_open()) {
				try {
					Json cj;
					cfg >> cj;
					if (cj.contains("inputMappings")) {
						j["inputMappings"] = cj["inputMappings"];
					}
				} catch (const std::exception& e) {
					ST_CORE_WARN("config.json 解析失败（不影响主配置）: {}", e.what());
				}
			}
		}

		loadFromJson(j);
		return true;
	}

	void Config::loadFromJson(const Json& j) {
		try {
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
				unsigned int fps = j["window"]["targetFPS"].get<unsigned int>();
				m_windowConfig.targetFPS = (fps > 0) ? fps : 144;
			}
		}
		if (j.contains("inputMappings")) {
			const auto& im = j["inputMappings"];

			// 热加载：先清空旧映射，避免残留
			m_inputMappings.actions.clear();
			m_inputMappings.axes.clear();

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
		} catch (const std::exception& e) {
			ST_CORE_WARN("配置文件字段解析失败（已保留已解析部分）: {}", e.what());
		}
	}

	Config& Config::GetInstance() {
		return EngineContext::current().config;
	}
} // namespace Shit