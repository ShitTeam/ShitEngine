#pragma once
#include <glm/glm.hpp>
#include <SFML/Graphics.hpp>

namespace Shit {
	using Vector2 = glm::vec2;
	using Vector3 = glm::vec3;
	using Matrix4 = glm::mat4;

	namespace Math {
		// SFML 和 GLM 的转换函数
		inline sf::Vector2f ToSF(const Shit::Vector2& v) { return { v.x, v.y }; }
		inline Shit::Vector2 ToGLM(const sf::Vector2f& v) { return { v.x, v.y }; }
	}
}