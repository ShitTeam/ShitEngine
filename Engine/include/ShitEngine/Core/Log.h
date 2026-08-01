#pragma once

#include <memory>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>
#include "Core.h"

namespace Shit {
	/**
	 * @brief 日志封装类
	 */
	class SHIT_API Log {
	public:
		static bool Init(); //初始化日志

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

// Core 日志宏（空指针安全：Log::Init 之前调用时静默跳过，避免引擎上下文构造期间崩溃）
#define ST_CORE_TRACE(...)   do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->trace(__VA_ARGS__); } while (0)
#define ST_CORE_DEBUG(...)   do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->debug(__VA_ARGS__); } while (0)
#define ST_CORE_INFO(...)    do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->info(__VA_ARGS__); } while (0)
#define ST_CORE_WARN(...)    do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->warn(__VA_ARGS__); } while (0)
#define ST_CORE_ERROR(...)   do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->error(__VA_ARGS__); } while (0)
#define ST_CORE_CRITICAL(...) do { if (auto lg = ::Shit::Log::GetCoreLogger()) lg->critical(__VA_ARGS__); } while (0)

//Client 日志宏（空指针安全）
#define ST_TRACE(...)   do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->trace(__VA_ARGS__); } while (0)
#define ST_DEBUG(...)   do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->debug(__VA_ARGS__); } while (0)
#define ST_INFO(...)    do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->info(__VA_ARGS__); } while (0)
#define ST_WARN(...)    do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->warn(__VA_ARGS__); } while (0)
#define ST_ERROR(...)   do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->error(__VA_ARGS__); } while (0)
#define ST_CRITICAL(...) do { if (auto lg = ::Shit::Log::GetClientLogger()) lg->critical(__VA_ARGS__); } while (0)