#pragma once

/**
 * @file ShitEngine.h
 * @brief 消费者统一入口——include 本文件即获得引擎全部公共头（Core/场景/对象/
 *        组件/渲染/动画/音频/输入/事件/资源/反射/UI/物理/插件）。
 */
// Core
#include "ShitEngine/Core/Core.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/TextInputGate.h"

// Event
#include "ShitEngine/Event/Event.h"
#include "ShitEngine/Event/EventBus.h"

// Audio
#include "ShitEngine/Audio/AudioPlayer.h"
#include "ShitEngine/Audio/AudioSource.h"

// Math
#include "ShitEngine/Math.h"

// Input
#include "ShitEngine/Input/Input.h"

// Scene
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Scene/SceneSerializer.h"

// GameObject
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/GameObject/ComponentRef.h"
#include "ShitEngine/GameObject/Prefab.h"

// Plugin（Runtime 与编辑器共享的动态插件加载）
#include "ShitEngine/Plugin/PluginManager.h"

// Component
#include "ShitEngine/Component/Component.h"
#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/Component/SpriteRenderer.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Component/AnimationComponent.h"
#include "ShitEngine/Component/Tilemap.h"

// Render
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Animation/AnimationClip.h"
#include "ShitEngine/Animation/Animator.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Render/Animation.h"
#include "ShitEngine/Render/SpriteSheet.h"

// Resource
#include "ShitEngine/Resource/ResourceManager.h"

// Reflection
#include "ShitEngine/Reflection/TypeInfo.h"
#include "ShitEngine/Reflection/TypeRegistry.h"
#include "ShitEngine/Reflection/Macros.h"

// UI
#include "ShitEngine/UI/UITransform.h"
#include "ShitEngine/UI/UIRendererComponent.h"
#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/UI/UIImage.h"
#include "ShitEngine/UI/UIText.h"
#include "ShitEngine/UI/UIButton.h"
#include "ShitEngine/UI/UIRenderSystem.h"
#include "ShitEngine/UI/UITextInput.h"
#include "ShitEngine/UI/UITextBox.h"
#include "ShitEngine/UI/UITextArea.h"

// Physics
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Physics/Joint2D.h"
#include "ShitEngine/Physics/BoxCollider2D.h"
#include "ShitEngine/Physics/CircleCollider2D.h"
