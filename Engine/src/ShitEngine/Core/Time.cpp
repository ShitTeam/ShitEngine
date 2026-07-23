#include "ShitEngine/Core/Time.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

#include <algorithm>

namespace Shit {
	Time& Time::GetInstance() {
		static Time instance;
		return instance;
	}

	Time::Time() = default;

	Time::~Time() = default;

	void Time::init() { // 初始化
		m_lastTime = SDL_GetTicksNS();
		m_currentTime = m_lastTime;
		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0;

		ST_CORE_TRACE("Time 初始化成功！");
	}

	void Time::update() {
		m_currentTime = SDL_GetTicksNS();

		// 先限制帧率：确保上一帧结束后已过至少 targetFrameTime
		if (m_targetFPS > 0) {
			Uint64 frameElapsed = m_currentTime - m_lastTime;
			Uint64 targetFrameTime = static_cast<Uint64>(1.0e9 / static_cast<double>(m_targetFPS));

			if (frameElapsed < targetFrameTime) {
				Uint64 remaining = targetFrameTime - frameElapsed;

				// 混合等待：先 sleep 大部分时间，再忙等剩余 ~1ms，
				// 避免 SDL_DelayNS 在 Windows 默认定时器精度（~15.6ms）下大幅超时
				constexpr Uint64 BUSY_WAIT_THRESHOLD = 1500000; // 1.5ms
				if (remaining > BUSY_WAIT_THRESHOLD) {
					SDL_DelayNS(remaining - BUSY_WAIT_THRESHOLD);
				}

				// 忙等剩余时间，保证精确帧间隔
				while ((SDL_GetTicksNS() - m_lastTime) < targetFrameTime) {
					// 自旋等待
				}
			}
		}

		// 再计算 deltaTime（包含等待时间），并钳制上限防止卡顿导致模拟爆炸
		m_currentTime = SDL_GetTicksNS();
		m_deltaTime = (std::min)(
			static_cast<float>(m_currentTime - m_lastTime) / 1000000000.0,
			0.1  // 钳制上限 100ms，避免窗口拖拽/断点等导致的巨大 dt
		);
		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0;

		m_lastTime = m_currentTime;
	}
}
