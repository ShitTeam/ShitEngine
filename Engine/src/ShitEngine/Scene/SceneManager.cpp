#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Scene/SceneSerializer.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"

#include <fstream>

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

	bool SceneManager::loadSceneFromFile(const std::string& path) {
		ST_CORE_INFO("正在从文件加载场景: {}", path);
		std::ifstream file(path);
		if (!file.is_open()) {
			ST_CORE_ERROR("[SceneManager] 无法打开场景文件: {}", path);
			return false;
		}

		nlohmann::json doc;
		try {
			file >> doc;
		} catch (const std::exception& e) {
			ST_CORE_ERROR("[SceneManager] 解析场景文件失败 ({}): {}", path, e.what());
			return false;
		}

		// 场景名取文件名（去扩展名）
		std::string name = path;
		if (auto slash = name.find_last_of("/\\"); slash != std::string::npos)
			name = name.substr(slash + 1);
		if (auto dot = name.find_last_of('.'); dot != std::string::npos)
			name = name.substr(0, dot);

		auto scene = std::make_unique<Scene>(name);
		scene->init();
		try {
			SceneSerializer::fromJson(doc, scene.get());
		} catch (const std::exception& e) {
			ST_CORE_ERROR("[SceneManager] 实例化场景失败 ({}): {}", path, e.what());
			return false;
		}

		loadScene(std::move(scene));
		return true;
	}

	void SceneManager::update() {
		// 上一帧 update 期间请求的延迟切换，先于本帧 update 执行
		if (m_pendingScene) {
			applyLoadScene(std::move(m_pendingScene));
		}

		m_isUpdating = true;
		if (m_currentScene) {
			m_currentScene->update();
		}
		m_isUpdating = false;

		// 本帧 update 期间（系统/组件回调内）请求的 LoadScene 延迟到 update 结束后生效：
		// 若立即销毁当前场景，仍在执行中的 Scene::update()/System::update() 栈帧会解引用
		// 已释放的场景内存（UAF）。
		if (m_pendingScene) {
			applyLoadScene(std::move(m_pendingScene));
		}
	}

	void SceneManager::destroy() {
		ST_CORE_TRACE("正在销毁场景管理器。");
		if (m_currentScene) {
			ST_CORE_DEBUG("正在清理场景 {} 。", m_currentScene->getName());
			// 组件 onDestroy 内可能误调 LoadScene：置 updating 使切换延迟到本方法之后，
			// 避免在场景销毁过程中重入 applyLoadScene 造成双重销毁
			m_isUpdating = true;
			m_currentScene->destroy();
			m_isUpdating = false;
			m_currentScene.reset();
		}
		// 丢弃未生效的延迟切换
		if (m_pendingScene) {
			m_pendingScene->destroy();
			m_pendingScene.reset();
		}
	}

	void SceneManager::loadScene(std::unique_ptr<Scene>&& scene) {
		if (!scene) {
			ST_CORE_WARN("试图加载空场景！");
			return;
		}

		// 场景 update / 销毁期间调用：延迟到 update 结束后（或下帧开头）生效，
		// 避免当前场景自毁 UAF / 重入双重销毁
		if (m_isUpdating) {
			ST_CORE_DEBUG("场景 update 期间请求加载 {} ，延迟到本帧结束后生效。", scene->getName());
			m_pendingScene = std::move(scene);
			return;
		}

		applyLoadScene(std::move(scene));
	}

	void SceneManager::applyLoadScene(std::unique_ptr<Scene>&& scene) {
		if (!scene) return;

		// 销毁当前场景（单一场景模型：切换即替换）。
		// 组件 onDestroy 内可能误调 LoadScene：置 updating 使嵌套切换也延迟，防重入
		if (m_currentScene) {
			ST_CORE_DEBUG("正在销毁当前场景 {} 。", m_currentScene->getName());
			m_isUpdating = true;
			m_currentScene->destroy();
			m_isUpdating = false;
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
