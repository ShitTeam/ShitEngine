#pragma once

#include <memory>
#include <optional>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include "../Core/Core.h"
#include "../Math.h"

namespace Shit {
	class Sprite;
	class CameraComponent;

	/**
	 * @brief 渲染器，封装 SDL3 绘制 API
	 *
	 * 提供清屏、绘制精灵和呈现最终图像的方法。
	 */
	class SHIT_API Renderer final {
	public:
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) = default;

		// --- 静态API ---
		static Renderer& GetInstance();
		static bool Init() { return GetInstance().init(); }
		static void LimitFPS() { GetInstance().limitFPS(); }
		static void ClearScreen() { GetInstance().clearScreen(); }
		static void Present() { GetInstance().present(); }
		static SDL_Renderer* GetRenderer() { return GetInstance().m_renderer.get(); }
		static unsigned int GetTargetFPS() { return GetInstance().getTargetFPS(); }
		static void SetTargetFPS(unsigned int fps) { GetInstance().setTargetFPS(fps); }

		// 逻辑分辨率
		static int GetLogicalWidth() { return GetInstance().m_logicalWidth; }
		static int GetLogicalHeight() { return GetInstance().m_logicalHeight; }

		/**
		 * @brief 在屏幕坐标中直接绘制一个精灵（UI 用）
		 * @param sprite 精灵数据
		 * @param position 屏幕坐标左上角
		 * @param size 可选目标尺寸，留空用纹理原始尺寸
		 */
		static void DrawSprite(const Sprite& sprite, const Vector2& position, const std::optional<Vector2>& size = std::nullopt);

	private:
		Renderer() = default;
		~Renderer() = default;

		bool init();
		void limitFPS();
		void clearScreen();
		void present();

		unsigned int getTargetFPS() const { return m_targetFPS; }
		void setTargetFPS(unsigned int fps) { m_targetFPS = fps; }

		struct SDLRendererDeleter {
			void operator()(SDL_Renderer* renderer) const {
				if (renderer) {
					SDL_DestroyRenderer(renderer);
				}
			}
		};

		std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
		unsigned int m_targetFPS = 144;
		Uint64 m_lastFrameTimeNS = 0;
		int m_logicalWidth = 1280;
		int m_logicalHeight = 720;
	};
}
