#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

#include <spdlog/sinks/base_sink.h>
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

			// ── 文件日志 sink：写入当前项目 .shitengine/log/ 目录 ──
			// 以进程工作目录为基准（Runtime 启动时 chdir 到 exe 目录，编辑器也运行在项目根）
			// 文件按"进程启动时间"归档，便于排查崩溃：log_YYYYMMDD_HHMMSS.txt
			std::filesystem::path logDir = std::filesystem::current_path() / ".shitengine" / "log";
			std::error_code ec;
			std::filesystem::create_directories(logDir, ec);

			std::string fileStem = "log";
			{
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
				fileStem = std::string("log_") + buf;
			}
			std::string logPath = (logDir / (fileStem + ".txt")).string();

			// 多 sink：控制台（彩色）+ 文件（完整等级）
			auto coreConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			auto clientConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, false);
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
}