#pragma once
#include <SFML/Graphics.hpp>
#include "Core.h"
#include "pch.h"

namespace Shit {
	class SHIT_API Game {
	public:
		Game();
		~Game();

		//初始化游戏
		void init(unsigned int width, unsigned int height, std::string title);

		//启动游戏
		void run();

	private:
		// --- 处理事件 ---
		void handleEvent(const auto&); // 默认事件
		void handleEvent(const sf::Event::Closed&); // 窗口关闭事件
		void handleEvent(const sf::Event::KeyPressed& keyPressed); // 按键被按下
		void handleEvent(const sf::Event::KeyReleased& keyReleased); // 按键被释放
		void handleEvent(const sf::Event::MouseButtonPressed& mouseButtonPressed); // 鼠标按键被按下
		void handleEvent(const sf::Event::MouseButtonReleased& mouseButtonReleased); // 鼠标按键被释放

		void update(); // 更新游戏状态
		void render(); // 渲染游戏画面
	};
}