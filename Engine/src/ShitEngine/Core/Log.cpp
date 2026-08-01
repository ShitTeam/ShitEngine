#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	bool Log::Init() {
		// 幂等：spdlog 全局 registry 按名查重，同名 logger 重复创建会抛异常。
		// Game::Init 失败重试或 Destroy 后重新初始化时必须能再次成功。
		// 部分失败（core 创建成功、client 抛异常）后重试：已存在的用 spdlog::get 取回，
		// 只补建缺失的，避免"logger already exists"永久失败。
		if (s_CoreLogger && s_ClientLogger) return true;

		try {
			// 设置日志格式：[时间] [日志名] [等级] 内容
			spdlog::set_pattern("%^[%T] %n: %v%$");

			if (!s_CoreLogger) {
				s_CoreLogger = spdlog::get("Shit");
				if (!s_CoreLogger) s_CoreLogger = spdlog::stdout_color_mt("Shit");
				if (s_CoreLogger) s_CoreLogger->set_level(spdlog::level::trace);
			}

			if (!s_ClientLogger) {
				s_ClientLogger = spdlog::get("App");
				if (!s_ClientLogger) s_ClientLogger = spdlog::stdout_color_mt("App");
				if (s_ClientLogger) s_ClientLogger->set_level(spdlog::level::trace);
			}
		}
		catch(const spdlog::spdlog_ex& e){
			std::cout << "日志初始化失败：" << e.what() << '\n';
			return false;
		}

		return s_CoreLogger && s_ClientLogger;
	}
}