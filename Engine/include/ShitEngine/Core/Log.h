#pragma once

#include <memory>
#include <utility>
#include <cstdlib>
#include <functional>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
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

		// --- 日志转发（编辑器日志面板接入） ---
		/// 日志消息回调：isCore = 引擎日志（Shit）/ 用户日志（App）；level = spdlog 等级；message = 文本
		using MessageCallback = std::function<void(bool isCore, int level, const std::string& message)>;
		/// 注册全局日志转发回调（Init 前/后均可；重新注册替换旧回调；传 nullptr 解除转发）
		static void SetMessageCallback(MessageCallback cb);
		/// 当前回调（供引擎内部 sink 读取）
		static const MessageCallback& GetMessageCallback() { return s_messageCallback; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static MessageCallback s_messageCallback;
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

// ════════════════════════════════════════════════════════════
// 断言（错误处理契约的"逻辑不变量"层）
//
// 契约约定：
//   - 致命初始化失败     → return false + ST_CORE_ERROR
//   - 非致命降级         → return nullptr + ST_CORE_WARN
//   - 逻辑不变量（不应发生）→ ST_CORE_ASSERT（Debug 拦截，Release 编译为 no-op）
//
// 用于"若此处为假则代码逻辑一定有 bug"的检查，如"System 必须有 scene"。
// 不要用于用户输入/可恢复错误（那应该走 WARN + fallback）。
// ════════════════════════════════════════════════════════════
#ifdef NDEBUG
	// Release：断言编译为 no-op
	#define ST_CORE_ASSERT(cond, msg) ((void)0)
	#define ST_ASSERT(cond, msg)      ((void)0)
#else
	// Debug：cond 为假 → 记录 CRITICAL 并 abort
	#define ST_CORE_ASSERT(cond, msg) \
		do { if (!(cond)) { \
			if (auto lg = ::Shit::Log::GetCoreLogger()) \
				lg->critical("ASSERT FAILED: {} -- {}", std::string(#cond), std::string(msg)); \
			std::abort(); \
		} } while (0)
	#define ST_ASSERT(cond, msg) \
		do { if (!(cond)) { \
			if (auto lg = ::Shit::Log::GetClientLogger()) \
				lg->critical("ASSERT FAILED: {} -- {}", std::string(#cond), std::string(msg)); \
			std::abort(); \
		} } while (0)
#endif