#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/SceneManager.h"

namespace Shit {
	RenderSystem& RenderSystem::GetInstance() {
		static RenderSystem instance;
		return instance;
	}

	sf::FloatRect RenderSystem::getViewBounds(const sf::View &view) {
		sf::Vector2f center = view.getCenter();
		sf::Vector2f size = view.getSize();
		// 计算左上角坐标
		return {{center.x - size.x / 2.f, center.y - size.y / 2.f}, size};
	}

	bool RenderSystem::init() { // 初始化
		m_window = Window::GetWindow(); // 获取Window
		return true;
	}

	void RenderSystem::drawSprite(const Sprite& sprite, const Vector2& position, const Vector2& scale, float angle)
	{
		auto texture = ResourceManager::GetTexture(sprite.getTexturePath());
		if (!texture) {
			ST_CORE_ERROR("无法获取路径为 {} 的纹理！", sprite.getTexturePath());
			return;
		}

		// 设置精灵
		sf::Sprite sf_sprite(*texture);
		sf_sprite.setPosition({ position.x, position.y });
		sf_sprite.setScale({ scale.x, scale.y });
		sf_sprite.setRotation(sf::degrees(angle));

		// 获取精灵在世界坐标系中的矩形
		sf::FloatRect bounds = sf_sprite.getGlobalBounds();

		// 获取相机在世界坐标系中的矩形
		sf::FloatRect viewBounds = getViewBounds(m_window->getView());

		if (viewBounds.findIntersection(bounds).has_value()) { // 如果有交集，则渲染
			m_window->draw(sf_sprite);
		}
	}

	void RenderSystem::setView(const sf::View &view) const {
		m_window->setView(view);
	}

	void RenderSystem::render() {
		m_window->clear(sf::Color::Black); //清屏，设置背景色为黑色

		SceneManager::Render();

		m_window->display(); //显示渲染结果
	}
}
