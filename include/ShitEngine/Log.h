#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>
#include "ShitEngine/Config.h"
#include "ShitEngine/pch.h"

namespace Shit {
	class SHIT_API Log {
	public:
		static void Init(); //初始化日志

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core 日志宏
#define ST_CORE_INFO(...)	::Shit::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ST_CORE_TRACE(...)  ::Shit::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ST_CORE_WARN(...)	::Shit::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ST_CORE_ERROR(...)  ::Shit::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ST_CORE_FATAL(...)  ::Shit::Log::GetCoreLogger()->fatal(__VA_ARGS__)

//Client 日志宏
#define ST_INFO(...)	::Shit::Log::GetClientLogger()->info(__VA_ARGS__)
#define ST_TRACE(...)	::Shit::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ST_WARN(...)	::Shit::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ST_ERROR(...)	::Shit::Log::GetClientLogger()->error(__VA_ARGS__)
#define ST_FATAL(...)	::Shit::Log::GetClientLogger()->fatal(__VA_ARGS__)