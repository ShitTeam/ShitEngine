#pragma once
#include <SFML/Graphics.hpp>
#include "Config.h"
#include "Log.h"
#include "pch.h"

namespace Shit {
	class SHIT_API Game {
	public:
		Game(const std::string& _title, const unsigned int& _width, const unsigned int& _height);
		~Game();

		//启动游戏
		void run();

	private:
		void input();  //处理输入
		void update(sf::Time& deltaTime); //更新游戏状态
		void render(); //渲染游戏画面

		sf::RenderWindow window; //游戏窗口
		sf::Clock clock;
	};
}