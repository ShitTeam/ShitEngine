#pragma once
#include <string>
#include <unordered_map>
#include <vector>
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
	 * @brief 动作绑定配置
	 *
	 * settings.json → inputMappings.actions：
	 *   "Jump": ["Space", "MouseButton.Left"]
	 *   "Attack": ["J"]
	 */
	struct ActionBinding {
		std::vector<std::string> keys; ///< 绑定的按键名（SDL scancode 名，如 "Space"/"Left Shift"）
	};

	/**
	 * @brief 轴绑定配置
	 *
	 * settings.json → inputMappings.axes：
	 *   "Horizontal": { "negative": ["A"], "positive": ["D"] }
	 */
	struct AxisBinding {
		std::vector<std::string> negative; ///< 负向按键
		std::vector<std::string> positive; ///< 正向按键
	};

	/**
	 * @brief 输入映射配置
	 *
	 * settings.json 中的 inputMappings 段：
	 * {
	 *   "actions": {
	 *     "Jump":   ["Space"],
	 *     "Attack": ["J", "E"],
	 *     "Sprint": ["Left Shift"]
	 *   },
	 *   "axes": {
	 *     "Horizontal": { "negative": ["A"], "positive": ["D"] },
	 *     "Vertical":   { "negative": ["S"], "positive": ["W"] }
	 *   }
	 * }
	 *
	 * 键名使用 SDL 官方 scancode 名；也接受无空格别名（LeftShift → Left Shift）。
	 * 鼠标：MouseButton.Left / Right / Middle / XButton1 / XButton2
	 */
	struct InputMappingsConfig {
		std::unordered_map<std::string, ActionBinding> actions; ///< 动作绑定
		std::unordered_map<std::string, AxisBinding> axes;      ///< 轴绑定
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
		inline static const ProjectConfig& GetProjectConfig() { return GetInstance().m_projectConfig; }
		inline static const WindowConfig& GetWindowConfig() { return GetInstance().m_windowConfig; }
		inline static const InputMappingsConfig& GetInputMappingsConfig() { return GetInstance().m_inputMappings; }

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;
		Config(Config&&) = delete;
		Config& operator=(Config&&) = delete;

	private:
		Config() = default;
		~Config() = default;

		ProjectConfig       m_projectConfig;
		WindowConfig        m_windowConfig;
		InputMappingsConfig m_inputMappings;
	};
} // namespace Shit
