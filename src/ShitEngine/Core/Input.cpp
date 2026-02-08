#include "ShitEngine/Core/Input.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	Input::Input() {

	}

	Input::~Input() {

	}

	Input& Input::GetInstance() { // 获取单例
		static Input instance;
		return instance;
	}

	bool Input::isKeyDown(Scancode code) { // 按键被按下
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Down)
			return true;
		else
			return false;
	}

	bool Input::isKeyPressed(Scancode code) { // 按键被持续按下
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Pressed)
			return true;
		else
			return false;
	}

	bool Input::isKeyReleased(Scancode code) { // 按键被释放
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Released)
			return true;
		else
			return false;
	}

	bool Input::isKeyDown(Mouse code)
	{
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Down)
			return true;
		else
			return false;
	}

	bool Input::isKeyPressed(Mouse code)
	{
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Pressed)
			return true;
		else
			return false;
	}

	bool Input::isKeyReleased(Mouse code)
	{
		if (auto it = m_keyStates.find(code); it != m_keyStates.end() && it->second == KeyState::Released)
			return true;
		else
			return false;
	}

	void Input::update() { // 更新
		for (auto& it : m_keyStates) {
			if (it.second == KeyState::Released) { // 如果是Released，则变成None
				it.second = KeyState::None;
			}
			else if (it.second == KeyState::Down) {
				// 因为操作系统会在按键按下后等待一段时间才持续发送Pressed事件
				// 所以在下一帧手动将Down 改为Pressed，就可以让 Down 事件只在第一帧时有效
				// （因为这个原因，我调了半天  Q^Q ）

				it.second = KeyState::Pressed;
			}
		}
	}

	void Input::handleEvent(const sf::Event::KeyPressed& keyPressed) {
		if (auto it = m_keyStates.find(keyPressed.scancode); it != m_keyStates.end()) {
			if (it->second == KeyState::None) { // 如果上一帧为None，那么下一帧为Down
				it->second = KeyState::Down;

				//ST_CORE_DEBUG("Down!!");

			}
			else if (it->second == KeyState::Down) { // 如果上一帧为Down，那么下一帧为Pressed
				it->second = KeyState::Pressed;

				//ST_CORE_DEBUG("Pressed!!");

			}
		}
		else { // 如果不存在，则创建Down
			m_keyStates.insert({ keyPressed.scancode, KeyState::Down });
			//ST_CORE_DEBUG("Down!!");
		}
	}

	void Input::handleEvent(const sf::Event::KeyReleased& keyReleased) {
		if (auto it = m_keyStates.find(keyReleased.scancode); it != m_keyStates.end()) {
			it->second = KeyState::Released;
			//ST_CORE_DEBUG("Released!!");
		}
		else {
			m_keyStates.insert({ keyReleased.scancode, KeyState::Released });
			//ST_CORE_DEBUG("Released!!");
		}
	}

	void Input::handleEvent(const sf::Event::MouseButtonPressed& mouseButtonPressed) {
		if (auto it = m_keyStates.find(mouseButtonPressed.button); it != m_keyStates.end()) {
			if (it->second == KeyState::None) {
				it->second = KeyState::Down;
			}
			else if (it->second == KeyState::Down) {
				it->second = KeyState::Pressed;
			}
		}
		else {
			m_keyStates.insert({ mouseButtonPressed.button, KeyState::Down });
		}
	}

	void Input::handleEvent(const sf::Event::MouseButtonReleased& mouseButtonReleased) {
		if (auto it = m_keyStates.find(mouseButtonReleased.button); it != m_keyStates.end()) {
			it->second = KeyState::Released;
		}
		else {
			m_keyStates.insert({ mouseButtonReleased.button, KeyState::Released });
		}
	}
}