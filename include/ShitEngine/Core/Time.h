#pragma once
#include "Core.h"
#include <SDL3/SDL_stdinc.h>

namespace Shit {
	class Game;

	/**
	 * @brief 静态时间类
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
		static void Init() { GetInstance().init(); }
		static void Update() { GetInstance().update(); }
		static float GetDeltaTime() { return GetInstance().m_deltaTime; }
		static double GetTotalTime() { return GetInstance().m_totalTime; }
		static unsigned int GetTargetFPS() { return GetInstance().m_targetFPS; }
		static void SetTargetFPS(unsigned int fps) { GetInstance().m_targetFPS = fps; }
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
