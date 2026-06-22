#include "ShitEngine/Core/Time.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

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
		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0f;

		ST_CORE_TRACE("Time 初始化成功！");
	}

	void Time::update() {
		m_currentTime = SDL_GetTicksNS();

		m_deltaTime = static_cast<float>(m_currentTime - m_lastTime) / 1000000000.0f;

		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0f;

		m_lastTime = m_currentTime;
	}
}