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
		static SceneManager instance;
		return instance;
	}

	void SceneManager::update() {
		Scene* scene = getCurrentScene(); // 获取当前场景
		if (scene) {
			scene->update();
		}
		
		processPendingActions();
	}

	void SceneManager::destroy() {
		ST_CORE_TRACE("正在销毁场景管理器。");
		while (!m_sceneStack.empty()) {
			if (m_sceneStack.back()) {
				spdlog::debug("正在清理场景 {} 。", m_sceneStack.back()->getName());
				m_sceneStack.back()->destroy();
			}
			m_sceneStack.pop_back();
		}
	}

	void SceneManager::processPendingActions()
	{
		for (auto& action : m_pendingActions) {
			switch (action.action)
			{
			case StackAction::Push:
				processPushScene(std::move(action.scene));
				break;
			case StackAction::Pop:
				processPopScene();
				break;
			case StackAction::Clear:
				processClearScene();
				break;
			case StackAction::Replace:
				processReplaceScene(std::move(action.scene));
				break;
			default:
				break;
			}
		}
		m_pendingActions.clear();
	}

	void SceneManager::processPushScene(std::unique_ptr<Scene>&& scene)
	{
		if (!scene) {
			ST_CORE_WARN("试图将空场景压入场景栈！");
			return;
		}

		ST_CORE_DEBUG("正在将场景 {} 压入场景栈。", scene->getName());

		m_sceneStack.push_back(std::move(scene));
	}

	void SceneManager::processPopScene()
	{
		if (m_sceneStack.empty()) {
			ST_CORE_WARN("场景栈为空，无法弹出场景！");
			return;
		}

		ST_CORE_DEBUG("正在从场景栈中弹出场景 {} 。", m_sceneStack.back()->getName());

		if (m_sceneStack.back()) {
			m_sceneStack.back()->destroy();
		}
		m_sceneStack.pop_back();
	}

	void SceneManager::processClearScene()
	{
		if (m_sceneStack.empty()) {
			ST_CORE_WARN("场景栈已经是空的了！");
			return;
		}

		ST_CORE_DEBUG("正在清空场景栈。");

		while (!m_sceneStack.empty()) {
			if (m_sceneStack.back()) {
				m_sceneStack.back()->destroy();
			}
			m_sceneStack.pop_back();
		}
	}

	void SceneManager::processReplaceScene(std::unique_ptr<Scene>&& scene)
	{
		if (!scene) {
			ST_CORE_WARN("试图用空场景替换！");
			return;
		}

		ST_CORE_DEBUG("正在用场景 {} 替换掉所有场景。", scene->getName());

		processClearScene();

		m_sceneStack.push_back(std::move(scene));
	}

	Scene* SceneManager::getCurrentScene() const
	{
		if (m_sceneStack.empty()) {
			ST_CORE_WARN("场景栈为空，无法获取当前场景！");
			return nullptr;
		}

		return m_sceneStack.back().get();
	}

	void SceneManager::pushScene(std::unique_ptr<Scene>&& scene)
	{
		m_pendingActions.push_back({ StackAction::Push, std::move(scene) });
	}

	void SceneManager::popScene()
	{
		m_pendingActions.push_back({ StackAction::Pop, nullptr });
	}

	void SceneManager::clearScene()
	{
		m_pendingActions.push_back({ StackAction::Clear, nullptr });
	}

	void SceneManager::replaceScene(std::unique_ptr<Scene>&& scene)
	{
		m_pendingActions.push_back({ StackAction::Replace, std::move(scene) });
	}
}