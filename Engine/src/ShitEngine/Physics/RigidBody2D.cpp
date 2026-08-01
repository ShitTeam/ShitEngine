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
		if (m_bodyValid) {
			b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
			b2Body_SetType(bodyId, static_cast<b2BodyType>(static_cast<int>(type)));
		}
	}

	void RigidBody2D::applyForce(const Vector2& force, bool wake) {
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_ApplyForceToCenter(bodyId, { force.x, force.y }, wake);
	}

	void RigidBody2D::applyForceToCenter(const Vector2& force, bool wake) {
		applyForce(force, wake);
	}

	void RigidBody2D::applyImpulse(const Vector2& impulse, bool wake) {
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_ApplyLinearImpulseToCenter(bodyId, { impulse.x, impulse.y }, wake);
	}

	void RigidBody2D::setLinearVelocity(const Vector2& velocity) {
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Body_SetLinearVelocity(bodyId, { velocity.x, velocity.y });
	}

	Vector2 RigidBody2D::getLinearVelocity() const {
		if (!m_bodyValid) return { 0, 0 };
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		b2Vec2 v = b2Body_GetLinearVelocity(bodyId);
		return { v.x, v.y };
	}

	void RigidBody2D::setTransform(const Vector2& position, float rotationDegrees) {
		if (!m_bodyValid) return;
		b2BodyId bodyId = Internal::MakeBodyId(m_bodyIndex, m_bodyWorld0, m_bodyGeneration);
		// 传送/设置刚体位置与旋转（度→弧度）；同时唤醒，使动态/运动学刚体立即响应
		b2Body_SetTransform(bodyId, { position.x, position.y }, b2MakeRot(glm::radians(rotationDegrees)));
		b2Body_SetAwake(bodyId, true);
	}
}