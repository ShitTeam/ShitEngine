#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

#include <spdlog/sinks/base_sink.h>
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

			// 转发 sink：仅在首次创建时追加一次（重试/复用既有 logger 时幂等）
			if (s_CoreLogger)     attachLoggerSink(s_CoreLogger, true);
			if (s_ClientLogger)   attachLoggerSink(s_ClientLogger, false);
		}
		catch(const spdlog::spdlog_ex& e){
			std::cout << "日志初始化失败：" << e.what() << '\n';
			return false;
		}

		return s_CoreLogger && s_ClientLogger;
	}
}