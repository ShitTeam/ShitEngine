#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

#include <spdlog/sinks/base_sink.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>

#ifdef _WIN32
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#endif

namespace Shit {
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
	Log::MessageCallback Log::s_messageCallback;

	namespace {
		/// 文件日志目录（空 = 默认：进程工作目录/.shitengine/log）。SetLogDirectory 可覆盖。
		std::string g_logDirectory;

		/// 生成文件日志路径：目录 + 按"当前时间"归档的文件名 log_YYYYMMDD_HHMMSS.txt
		std::string buildLogFilePath(const std::string& dir) {
			std::filesystem::path logDir = dir.empty()
				? std::filesystem::current_path() / ".shitengine" / "log"
				: std::filesystem::path(dir);
			std::error_code ec;
			std::filesystem::create_directories(logDir, ec);

			auto now = std::chrono::system_clock::now();
			std::time_t t = std::chrono::system_clock::to_time_t(now);
			std::tm tm{};
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			localtime_r(&t, &tm);
#endif
			char buf[32];
			std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
			return (logDir / (std::string("log_") + buf + ".txt")).string();
		}

		/// 从 logger 摘除旧文件 sink（关闭旧文件；不触碰控制台/回调 sink）
		void detachFileSink(const std::shared_ptr<spdlog::logger>& logger) {
			if (!logger) return;
			auto& sinks = logger->sinks();
			sinks.erase(std::remove_if(sinks.begin(), sinks.end(),
				[](const std::shared_ptr<spdlog::sinks::sink>& s) {
					return dynamic_cast<spdlog::sinks::basic_file_sink_mt*>(s.get()) != nullptr;
				}), sinks.end());
		}

		/// 按当前 g_logDirectory 创建文件 sink（trace 级）
		std::shared_ptr<spdlog::sinks::basic_file_sink_mt> makeFileSink() {
			return std::make_shared<spdlog::sinks::basic_file_sink_mt>(buildLogFilePath(g_logDirectory), false);
		}
	}

	/// 把日志消息转发给外部回调（编辑器日志面板接入）
	class CallbackSink final : public spdlog::sinks::base_sink<std::mutex> {
	public:
		explicit CallbackSink(bool isCore) : m_isCore(isCore) {}

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override {
			const auto& cb = Log::GetMessageCallback();
			if (!cb) return;
			cb(m_isCore, static_cast<int>(msg.level),
			   std::string(msg.payload.data(), msg.payload.size()));
		}
		void flush_() override {}

	private:
		bool m_isCore = false;
	};

	static void attachLoggerSink(const std::shared_ptr<spdlog::logger>& logger, bool isCore) {
		if (!logger) return;
		// 幂等：已挂过 CallbackSink 则不再追加。否则部分失败（core 建好、client 抛异常）
		// 后重试 Init 会重复挂 sink → 编辑器日志面板每条日志显示多次
		for (const auto& sink : logger->sinks()) {
			if (dynamic_cast<CallbackSink*>(sink.get())) return;
		}
		logger->sinks().push_back(std::make_shared<CallbackSink>(isCore)); // Init 期（单线程）追加安全
	}

	void Log::SetMessageCallback(MessageCallback cb) {
		s_messageCallback = std::move(cb);
	}

	bool Log::Init() {
#ifdef _WIN32
		// Windows 控制台设为 UTF-8：中文日志在 Qt Creator 应用输出、独立 cmd 中均正常显示（默认 936 代码页会乱码）
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
#endif
		// 幂等：spdlog 全局 registry 按名查重，同名 logger 重复创建会抛异常。
		// Game::Init 失败重试或 Destroy 后重新初始化时必须能再次成功。
		// 部分失败（core 创建成功、client 抛异常）后重试：已存在的用 spdlog::get 取回，
		// 只补建缺失的，避免"logger already exists"永久失败。
		if (s_CoreLogger && s_ClientLogger) return true;

		try {
			// 设置日志格式：[时间] [日志名] [等级] 内容
			spdlog::set_pattern("%^[%T] %n: %v%$");
			// 每条日志立即落盘：文件日志用于崩溃排查，若等缓冲/正常退出才 flush，
			// 进程崩溃时缓冲内的最后几百条日志会全部丢失，文件形同虚设。
			// 同时设置 registry 级 flush_on（后续新建 logger 继承）与 logger 级
			// flush_on（对已复用/新建的 Shit/App logger 直接生效，双保险）。
			spdlog::flush_on(spdlog::level::trace);

			// ── 文件日志 sink：默认写入进程工作目录 .shitengine/log/ ──
			// （编辑器打开项目后经 Log::SetLogDirectory 切换到项目 .shitengine/log）
			// 文件按"当前时间"归档，便于排查崩溃：log_YYYYMMDD_HHMMSS.txt
			// 多 sink：控制台（彩色）+ 文件（完整等级）
			auto coreConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			auto clientConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			auto fileSink = makeFileSink();
			fileSink->set_level(spdlog::level::trace);

			if (!s_CoreLogger) {
				s_CoreLogger = spdlog::get("Shit");
				if (!s_CoreLogger) {
					s_CoreLogger = std::make_shared<spdlog::logger>(
						"Shit", spdlog::sinks_init_list{coreConsoleSink, fileSink});
					spdlog::register_logger(s_CoreLogger);
				}
				if (s_CoreLogger) s_CoreLogger->set_level(spdlog::level::trace);
			}

			if (!s_ClientLogger) {
				s_ClientLogger = spdlog::get("App");
				if (!s_ClientLogger) {
					s_ClientLogger = std::make_shared<spdlog::logger>(
						"App", spdlog::sinks_init_list{clientConsoleSink, fileSink});
					spdlog::register_logger(s_ClientLogger);
				}
				if (s_ClientLogger) s_ClientLogger->set_level(spdlog::level::trace);
			}

// 转发 sink：仅在首次创建时追加一次（重试/复用既有 logger 时幂等）
				if (s_CoreLogger)     attachLoggerSink(s_CoreLogger, true);
				if (s_ClientLogger)   attachLoggerSink(s_ClientLogger, false);

				// logger 级 flush_on：确保文件日志逐条落盘（即使 registry 级设置未正确传播）
				if (s_CoreLogger)     s_CoreLogger->flush_on(spdlog::level::trace);
				if (s_ClientLogger)   s_ClientLogger->flush_on(spdlog::level::trace);
		}
		catch(const spdlog::spdlog_ex& e){
			std::cout << "日志初始化失败：" << e.what() << '\n';
			return false;
		}

		return s_CoreLogger && s_ClientLogger;
	}

	void Log::SetLogDirectory(const std::string& dir) {
		g_logDirectory = dir;
		if (!s_CoreLogger && !s_ClientLogger) return;  // 未初始化：Init() 时按新目录创建

		// 已初始化：关闭旧文件 sink，在新目录下按当前时间重开归档文件
		detachFileSink(s_CoreLogger);
		detachFileSink(s_ClientLogger);
		try {
			auto fileSink = makeFileSink();
			fileSink->set_level(spdlog::level::trace);
			if (s_CoreLogger)   s_CoreLogger->sinks().push_back(fileSink);
			if (s_ClientLogger) s_ClientLogger->sinks().push_back(fileSink);
			ST_CORE_INFO("[Log] 文件日志目录: {}", g_logDirectory.empty()
				? (std::filesystem::current_path() / ".shitengine" / "log").string()
				: g_logDirectory);
		}
		catch (const spdlog::spdlog_ex& e) {
			std::cout << "重设日志目录失败：" << e.what() << '\n';
		}
	}
}