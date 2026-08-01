#pragma once

// 各子系统完整定义：EngineContext 按值持有全部实例
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/TextInputGate.h"
#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Audio/AudioPlayer.h"
#include "ShitEngine/Event/EventBus.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Reflection/TypeRegistry.h"

namespace Shit {
	class Log;  // 日志为进程级全局（纯静态），不入上下文

	/**
	 * @brief 引擎上下文：持有全部子系统实例
	 *
	 * 替代原先 12 个进程级单例的"引擎只能有一个实例"限制。一个进程可创建多个
	 * EngineContext（编辑器进程内预览、多实例对比、单元测试），每个拥有独立的
	 * Window/Input/Renderer/SceneManager 等完整子系统。静态门面（如
	 * Shit::Input::IsKeyDown）转发到当前上下文（EngineContext::current()）。
	 *
	 * 用法：
	 *   // 单实例（默认，行为与旧单例一致）——无需手动创建，第一次 current() 时懒创建
	 *   Shit::Game::Init(); Shit::Game::Run(); Shit::Game::Destroy();
	 *
	 *   // 多实例（编辑器预览）
	 *   Shit::EngineContext preview;
	 *   Shit::EngineContext::setCurrent(&preview);
	 *   Shit::Game::Init();  // 初始化 preview 的子系统
	 *   // ... 运行 ...
	 *   Shit::Game::Destroy();
	 *   Shit::EngineContext::setCurrent(&editorCtx);  // 切回编辑器上下文
	 */
	class SHIT_API EngineContext {
	public:
		EngineContext() = default;
		~EngineContext() = default;

		EngineContext(const EngineContext&) = delete;
		EngineContext& operator=(const EngineContext&) = delete;
		EngineContext(EngineContext&&) = delete;
		EngineContext& operator=(EngineContext&&) = delete;

		// 子系统实例（声明顺序 = 构造顺序，析构逆序；生命周期由 Game::Init/Run/Destroy 驱动）
		Game             game;
		Config           config;
		Window           window;
		Time             time;
		TextInputGate    textInputGate;
		Input            input;
		Renderer         renderer;
		ResourceManager  resources;
		AudioPlayer      audio;
		EventBus         eventBus;
		SceneManager     sceneManager;
		TypeRegistry     typeRegistry;

		// --- 当前上下文 ---
		/// @brief 当前活动上下文；未设置时懒创建一个进程默认上下文（与旧单例行为一致）
		static EngineContext& current();
		/// @brief 切换到指定上下文（多实例/编辑器预览用）
		static void setCurrent(EngineContext* ctx);
		/// @brief 恢复进程默认上下文
		static void resetCurrent();

	private:
		static EngineContext* s_current;
	};
}
