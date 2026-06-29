#include "ShitEngine/Core/Log.h"

namespace Shit {
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	bool Log::Init() {
		try {
			// 设置日志格式：[时间] [日志名] [等级] 内容
			spdlog::set_pattern("%^[%T] %n: %v%$");

			//初始化Core日志
			s_CoreLogger = spdlog::stdout_color_mt("Shit");
			s_CoreLogger->set_level(spdlog::level::trace);

			//初始化Client日志
			s_ClientLogger = spdlog::stdout_color_mt("App");
			s_ClientLogger->set_level(spdlog::level::trace);
		}
		catch(const spdlog::spdlog_ex& e){
			std::cout << "日志初始化失败：" << e.what() << '\n';
			return false;
		}
		
		return true;
	}
}