#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/CircleCollider2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	CircleCollider2D::CircleCollider2D() = default;

	CircleCollider2D::CircleCollider2D(float radius) : m_radius(std::max(1.0f, radius)) {}

	CircleCollider2D::~CircleCollider2D() = default;

	void CircleCollider2D::onAttach() {
		Component::onAttach();

		if (!m_owner) return;

		auto* rigidBody = m_owner->getComponent<RigidBody2D>();
		if (!rigidBody || !rigidBody->hasValidBody()) {
			ST_CORE_WARN("[CircleCollider2D] 未找到有效的 RigidBody2D，无法创建碰撞体");
			return;
		}

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.material.friction = m_friction;
		shapeDef.material.restitution = m_restitution;
		shapeDef.density = m_density;

		float radius = std::max(1.0f, m_radius);
		b2Circle circle = { { 0.0f, 0.0f }, radius };

		b2BodyId bodyId = Internal::MakeBodyId(
			rigidBody->getBodyIndex(),
			rigidBody->getBodyWorld0(),
			rigidBody->getBodyGeneration()
		);

		b2ShapeId id = b2CreateCircleShape(bodyId, &shapeDef, &circle);
		m_shapeIndex = id.index1;
		m_shapeWorld0 = id.world0;
		m_shapeGeneration = id.generation;
		m_shapeValid = true;
	}

	void CircleCollider2D::onDetach() {
		Component::onDetach();

		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2DestroyShape(shapeId, true);
			}
			m_shapeValid = false;
		}
	}

	void CircleCollider2D::onDestroy() {
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2DestroyShape(shapeId, true);
			}
			m_shapeValid = false;
		}
		Component::onDestroy();
	}

	void CircleCollider2D::setRadius(float radius) {
		m_radius = std::max(1.0f, radius);
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Circle circle = { { 0.0f, 0.0f }, m_radius };
				b2Shape_SetCircle(shapeId, &circle);
			}
		}
	}

	void CircleCollider2D::setDensity(float density) {
		m_density = density;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetDensity(shapeId, m_density, true);
			}
		}
	}

	void CircleCollider2D::setFriction(float friction) {
		m_friction = friction;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetFriction(shapeId, m_friction);
			}
		}
	}

	void CircleCollider2D::setRestitution(float restitution) {
		m_restitution = restitution;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetRestitution(shapeId, m_restitution);
			}
		}
	}
}