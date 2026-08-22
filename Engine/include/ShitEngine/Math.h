#pragma once
#include <glm/glm.hpp>

struct SDL_FRect;

namespace Shit {
	using Vector2 = glm::vec2;

	/// Rectangle struct (float precision), with SDL_FRect conversion
	struct Rect {
		float x = 0.0f;
		float y = 0.0f;
		float w = 0.0f;
		float h = 0.0f;

		Rect() = default;
		Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

		SDL_FRect toSDL() const;
		static Rect fromSDL(const SDL_FRect &r);
	};
}
