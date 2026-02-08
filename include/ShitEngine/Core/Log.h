#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>
#include "Core.h"
#include "pch.h"

namespace Shit {
	/**
	 * @brief 日志封装类
	 */
	class SHIT_API Log {
	public:
		static void Init(); //初始化日志

		//静态调用API
		template<typename... Args>
		inline static void Trace(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->trace(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Debug(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->debug(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Info(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->info(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Warn(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->warn(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Error(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->error(fmt, std::forward<Args>(args)...); }

		template<typename... Args>
		inline static void Critical(fmt::format_string<Args...> fmt, Args&&... args) { s_ClientLogger->critical(fmt, std::forward<Args>(args)...); }

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core 日志宏
#define ST_CORE_TRACE(...)  ::Shit::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ST_CORE_DEBUG(...)  ::Shit::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define ST_CORE_INFO(...)	::Shit::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ST_CORE_WARN(...)	::Shit::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ST_CORE_ERROR(...)  ::Shit::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ST_CORE_CRITICAL(...)  ::Shit::Log::GetCoreLogger()->critical(__VA_ARGS__)

//Client 日志宏
#define ST_TRACE(...)	::Shit::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ST_DEBUG(...)  ::Shit::Log::GetClientLogger()->debug(__VA_ARGS__)
#define ST_INFO(...)	::Shit::Log::GetClientLogger()->info(__VA_ARGS__)
#define ST_WARN(...)	::Shit::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ST_ERROR(...)	::Shit::Log::GetClientLogger()->error(__VA_ARGS__)
#define ST_CRITICAL(...)  ::Shit::Log::GetClientLogger()->critical(__VA_ARGS__)