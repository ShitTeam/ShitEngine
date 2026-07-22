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
	 * 管理逻辑分辨率、清屏、精灵绘制（UI 用）以及 Present。
	 * 默认逻辑分辨率 1280×720，通过 settings.json 中的 window.width/height 配置。
	 */
	class SHIT_API Renderer final {
	public:
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) = default;

		// --- 静态API ---
		static Renderer& GetInstance();
		static bool Init() { return GetInstance().init(); }                        ///< 初始化渲染器
		static void ClearScreen() { GetInstance().clearScreen(); }                 ///< 清屏（每帧开头调用）
		static void Present() { GetInstance().present(); }                         ///< 交换缓冲区（每帧末尾调用）
		static SDL_Renderer* GetRenderer() { return GetInstance().m_renderer.get(); } ///< 获取原生 SDL_Renderer

		static int GetLogicalWidth() { return GetInstance().m_logicalWidth; }      ///< 逻辑分辨率宽度
		static int GetLogicalHeight() { return GetInstance().m_logicalHeight; }    ///< 逻辑分辨率高度

		/**
		 * @brief 在屏幕坐标直接绘制精灵（UI 场景用）
		 * @param sprite  精灵数据
		 * @param position 屏幕坐标左上角
		 * @param size     可选目标尺寸，留空用纹理原始尺寸
		 */
		static void DrawSprite(const Sprite& sprite, const Vector2& position, const std::optional<Vector2>& size = std::nullopt);

	// --- 绘制原语（封装 SDL3 绘制 API，供 UI / HUD 使用）---
	static void SetDrawColor(const Color& color);   ///< 设置绘制颜色（SDL_SetRenderDrawColor）
	static void FillRect(const SDL_FRect& rect);     ///< 实心矩形（SDL_RenderFillRect）
	static void SetClipRect(const SDL_Rect* rect);  ///< 设置裁剪矩形（传 nullptr 清除）
	static void ClearClipRect();                    ///< 清除裁剪矩形
	static void SetViewport(const SDL_Rect* viewport); ///< 设置视口（传 nullptr 恢复全屏）
	static void ClearViewport();                    ///< 恢复全屏视口
	static void DrawTexture(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst); ///< 绘制纹理（SDL_RenderTexture）
	static void DrawTextureRotated(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst,
		double angle, const SDL_FPoint* center, SDL_FlipMode flip); ///< 旋转/翻转绘制（SDL_RenderTextureRotated）
	static void RenderCoordinatesToWindow(float x, float y, float* winX, float* winY); ///< 逻辑坐标 → 窗口物理像素坐标
	static SDL_Texture* CreateTextureFromSurface(SDL_Surface* surface); ///< surface → texture（SDL_CreateTextureFromSurface）

	private:
		Renderer() = default;
		~Renderer() = default;

		bool init();
		void clearScreen();
		void present();

		struct SDLRendererDeleter {
			void operator()(SDL_Renderer* renderer) const {
				if (renderer) SDL_DestroyRenderer(renderer);
			}
		};

		std::unique_ptr<SDL_Renderer, SDLRendererDeleter> m_renderer;
		int m_logicalWidth = 1280;
		int m_logicalHeight = 720;
	};
}
