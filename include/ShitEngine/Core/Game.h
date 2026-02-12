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
		bool init();

		//启动游戏
		void run();

		// --- 静态API ---
		static Game& GetInstance();
		inline static bool Init() { return GetInstance().init(); }
		inline static void Run() { GetInstance().run(); }
		static void Destroy(); // 销毁函数

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(Game&&) = delete;

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