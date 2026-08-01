#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
	SceneManager::SceneManager() {
		ST_CORE_TRACE("场景管理器已创建！");
	}

	SceneManager::~SceneManager()
	{
		ST_CORE_TRACE("场景管理器已销毁！");
	}

	SceneManager& SceneManager::GetInstance() {
		return EngineContext::current().sceneManager;
	}

	void SceneManager::update() {
		if (m_currentScene) {
			m_currentScene->update();
		}
	}

	void SceneManager::destroy() {
		ST_CORE_TRACE("正在销毁场景管理器。");
		if (m_currentScene) {
			ST_CORE_DEBUG("正在清理场景 {} 。", m_currentScene->getName());
			m_currentScene->destroy();
			m_currentScene.reset();
		}
	}

	void SceneManager::loadScene(std::unique_ptr<Scene>&& scene) {
		if (!scene) {
			ST_CORE_WARN("试图加载空场景！");
			return;
		}

		// 销毁当前场景（单一场景模型：切换即替换）
		if (m_currentScene) {
			ST_CORE_DEBUG("正在销毁当前场景 {} 。", m_currentScene->getName());
			m_currentScene->destroy();
		}

		// 自动初始化：防止漏调 scene->init() 导致场景没有任何 System
		if (!scene->hasSystems()) {
			scene->init();
		}

		ST_CORE_DEBUG("正在加载场景 {} 。", scene->getName());
		m_currentScene = std::move(scene);
	}

	Scene* SceneManager::getCurrentScene() const {
		return m_currentScene ? m_currentScene.get() : nullptr;
	}
}
