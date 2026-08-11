#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	RigidBody2D::RigidBody2D() = default;
	RigidBody2D::~RigidBody2D() = default;

	void RigidBody2D::onCreate() {
		Component::onCreate();
	}

	void RigidBody2D::onAttach() {
		Component::onAttach();

		// 广播给 Scene，由 PhysicsSystem2D 认领并创建物理体（解耦：不再查询具体系统类型）。
		// 系统未注册时 registerComponent 返回 false → m_isRegistered=false，后续可补挂。
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			m_isRegistered = scene->registerComponent(this);
			if (!m_isRegistered) {
				// 自愈：数据驱动场景（.scene 加载 / 编辑器场景编辑）不预注册物理系统
				//（P6/P7 迁移后场景 init 只带 Behavior/Render/UI 三系统），首个刚体挂载
				// 时自动补注册 PhysicsSystem2D 并重试认领——Runtime 与编辑器所有路径统一；
				// 已注册场景里重复挂刚体时 registerSystem 幂等返回现有系统。
				scene->registerSystem<PhysicsSystem2D>();
				m_isRegistered = scene->registerComponent(this);
			}
		} else {
			m_isRegistered = false;
		}
	}

	void RigidBody2D::onDetach() {
		Component::onDetach();

		// 广播卸下，由 PhysicsSystem2D 销毁物理体并注销
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			scene->unregisterComponent(this);
		}

		// 兜底：若物理体仍有效（系统未处理，如场景销毁时系统已先销毁），直接销毁
		if (m_bodyValid) {
			b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
			if (b2Body_IsValid(bodyId)) {
				b2DestroyBody(bodyId);
			}
			m_bodyValid = false;
		}
	}

	void RigidBody2D::onDestroy() {
		if (m_bodyValid) {
			ST_CORE_WARN("[RigidBody2D] 组件直接销毁而未 detach");
			auto* scene = m_owner ? m_owner->getScene() : nullptr;
			if (scene) {
				auto* physics = scene->getSystem<PhysicsSystem2D>();
				if (physics) {
					physics->unregisterRigidBody(this);
				}
			}
			b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
			if (b2Body_IsValid(bodyId)) {
				b2DestroyBody(bodyId);
			}
			m_bodyValid = false;
		}
		Component::onDestroy();
	}

	void RigidBody2D::setBodyType(Type type) {
		m_type = type;
		ensureBody();   // 物理 API 自愈：刚体尚未创建（Transform 后置挂载）时补建
		if (m_bodyValid) {
			b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
			b2Body_SetType(bodyId, static_cast<b2BodyType>(static_cast<int>(type)));
		}
	}

	void RigidBody2D::onFieldChanged(const std::string& fieldName) {
		// 检查器直写反射字段后，把改动同步到已创建的刚体；无体时等补建（createRigidBody
		// 读 m_* 字段）即正确。
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		if (!b2Body_IsValid(bodyId)) return;
		if (fieldName == "m_type") {
			b2Body_SetType(bodyId, static_cast<b2BodyType>(static_cast<int>(m_type)));
		} else if (fieldName == "m_gravityScale") {
			b2Body_SetGravityScale(bodyId, m_gravityScale);
		} else if (fieldName == "m_linearDamping") {
			b2Body_SetLinearDamping(bodyId, m_linearDamping);
		} else if (fieldName == "m_fixedRotation") {
			b2Body_SetFixedRotation(bodyId, m_fixedRotation);
		}
	}

	/// @brief 物理体尚未创建时补建（场景反序列化组件顺序不定：刚体先于 Transform 挂载
	/// 时，物理系统的每帧补建要等 BehaviorSystem 之后才跑，onStart 里调物理 API 会落空）。
	/// 幂等：已有效或无场景/无系统时直接返回。
	void RigidBody2D::ensureBody() {
		if (m_bodyValid) return;
		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (!scene) return;
		if (auto* physics = scene->getSystem<PhysicsSystem2D>()) {
			physics->createRigidBody(this);
		}
	}

	void RigidBody2D::applyForce(const Vector2& force, bool wake) {
		ensureBody();
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_ApplyForceToCenter(bodyId, { force.x, force.y }, wake);
	}

	void RigidBody2D::applyForceToCenter(const Vector2& force, bool wake) {
		applyForce(force, wake);
	}

	void RigidBody2D::applyImpulse(const Vector2& impulse, bool wake) {
		ensureBody();
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_ApplyLinearImpulseToCenter(bodyId, { impulse.x, impulse.y }, wake);
	}

	void RigidBody2D::setLinearVelocity(const Vector2& velocity) {
		ensureBody();
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_SetLinearVelocity(bodyId, { velocity.x, velocity.y });
	}

	Vector2 RigidBody2D::getLinearVelocity() const {
		// const 方法不建体：直接判有效返回（无体时视为静止）
		if (!m_bodyValid) return { 0, 0 };
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Vec2 v = b2Body_GetLinearVelocity(bodyId);
		return { v.x, v.y };
	}

	void RigidBody2D::setTransform(const Vector2& position, float rotationDegrees) {
		ensureBody();
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		// 传送/设置刚体位置与旋转（度→弧度）；同时唤醒，使动态/运动学刚体立即响应
		b2Body_SetTransform(bodyId, { position.x, position.y }, b2MakeRot(glm::radians(rotationDegrees)));
		b2Body_SetAwake(bodyId, true);
	}
}