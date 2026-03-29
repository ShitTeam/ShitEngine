#pragma once
#include "../Core/Core.h"
#include "../Math.h"

namespace sf {

}

namespace Shit {
	class Sprite;

	/**
	 * @brief 渲染器
	 */
	class SHIT_API RenderSystem final {
	public:
		// --- 静态API ---
		static RenderSystem& GetInstance();
		inline static bool Init() { return GetInstance().init(); }
		inline static void DrawSprite(const Sprite& sprite, const Vector2& position, const Vector2& scale = { 1.0f, 1.0f }, float angle = 0.0f) { GetInstance().drawSprite(sprite, position, scale, angle); }
		inline static void SetView(sf::View &view) { GetInstance().setView(std::forward<sf::View>(view)); }
		inline static void Render() { GetInstance().render(); }

	private:
		sf::FloatRect getViewBounds(const sf::View& view); // 获取相机在世界坐标系中的矩形

		bool init();

		/**
		 * @brief 绘制精灵
		 * @param sprite 精灵
		 * @param position 坐标
		 * @param scale 缩放
		 * @param angle 旋转角度
		 */
		void drawSprite(const Sprite& sprite, const Vector2& position, const Vector2& scale = { 1.0f, 1.0f }, float angle = 0.0f);

		void setView(const sf::View& view) const;

		void render(); // 渲染

		sf::RenderWindow* m_window = nullptr; // 缓存的窗口
	};
}