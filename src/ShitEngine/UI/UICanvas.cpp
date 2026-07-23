#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/Renderer.h"

namespace Shit {
	SDL_FRect UICanvas::getScreenRect() const {
		return SDL_FRect{ 0.0f, 0.0f,
			static_cast<float>(Renderer::GetLogicalWidth()),
			static_cast<float>(Renderer::GetLogicalHeight()) };
	}
}
