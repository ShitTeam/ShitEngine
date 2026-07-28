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

		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (!scene) return;

		auto* physics = scene->getSystem<PhysicsSystem2D>();
		if (!physics) {
			ST_CORE_WARN("[RigidBody2D] 场景中未找到 PhysicsSystem2D，无法创建物理体");
			return;
		}

		auto* transform = m_owner->getComponent<TransformComponent>();
		if (!transform) {
			ST_CORE_WARN("[RigidBody2D] 缺少 TransformComponent，无法创建物理体");
			return;
		}

		b2WorldId worldId = Internal::MakeWorldId(physics->m_worldIndex, physics->m_worldGeneration);
		if (!b2World_IsValid(worldId)) {
			ST_CORE_ERROR("[RigidBody2D] 物理世界无效");
			return;
		}

		b2BodyDef def = b2DefaultBodyDef();
		def.type = static_cast<b2BodyType>(static_cast<int>(m_type));
		// 注：b2SetLengthUnitsPerMeter 已设置，所有位置/速度单位为像素
		def.position = { transform->getPosition().x, transform->getPosition().y };
		def.rotation = b2MakeRot(transform->getRotation());
		def.gravityScale = m_gravityScale;
		def.linearDamping = m_linearDamping;
		def.fixedRotation = m_fixedRotation;

		b2BodyId id = b2CreateBody(worldId, &def);
		m_bodyIndex = id.index1;
		m_bodyWorld0 = id.world0;
		m_bodyGeneration = id.generation;
		m_bodyValid = true;

		physics->registerRigidBody(this);
	}

	void RigidBody2D::onDetach() {
		Component::onDetach();

		if (m_bodyValid) {
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
}