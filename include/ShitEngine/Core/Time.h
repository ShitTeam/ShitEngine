#pragma once
#include "Core.h"
#include <SDL3/SDL_stdinc.h>

namespace Shit {
	class Game;

	/**
	 * @brief 时间管理类
	 *
	 * 提供帧间时间差、总运行时间和帧率控制。
	 */
	class SHIT_API Time {
		friend class Game;
	public:
		Time(const Time&) = delete;
		Time& operator=(const Time&) = delete;
		Time(Time&&) = delete;
		Time& operator=(Time&&) = delete;

		// --- 静态API ---
		static Time& GetInstance();
		static void Init() { GetInstance().init(); }                    ///< 初始化计时器
		static void Update() { GetInstance().update(); }                ///< 每帧调用，更新 m_deltaTime
		static float GetDeltaTime() { return GetInstance().m_deltaTime; }   ///< 上一帧耗时（秒）
		static double GetTotalTime() { return GetInstance().m_totalTime; }  ///< 引擎启动至今的总时长（秒）
		static unsigned int GetTargetFPS() { return GetInstance().m_targetFPS; }  ///< 目标帧率上限
		static void SetTargetFPS(unsigned int fps) { GetInstance().m_targetFPS = fps; }  ///< 设帧率上限

	private:
		Time();
		~Time();

		void init();
		void update();

		float m_deltaTime = 0.0f;
		double m_totalTime = 0.0f;
		Uint64 m_lastTime;
		Uint64 m_currentTime;
		unsigned int m_targetFPS = 144;
	};
}
