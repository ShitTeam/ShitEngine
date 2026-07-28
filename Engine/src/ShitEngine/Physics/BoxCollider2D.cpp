#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/BoxCollider2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	BoxCollider2D::BoxCollider2D() = default;

	BoxCollider2D::BoxCollider2D(const Vector2& size) : m_size(size) {
		m_size.x = std::max(1.0f, m_size.x);
		m_size.y = std::max(1.0f, m_size.y);
	}

	BoxCollider2D::~BoxCollider2D() = default;

	void BoxCollider2D::onAttach() {
		Component::onAttach();

		if (!m_owner) return;

		auto* rigidBody = m_owner->getComponent<RigidBody2D>();
		if (!rigidBody || !rigidBody->hasValidBody()) {
			ST_CORE_WARN("[BoxCollider2D] 未找到有效的 RigidBody2D，无法创建碰撞体");
			return;
		}

		// b2SetLengthUnitsPerMeter 已设置，尺寸单位为像素
		b2Polygon box = b2MakeBox(m_size.x * 0.5f, m_size.y * 0.5f);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.material.friction = m_friction;
		shapeDef.material.restitution = m_restitution;
		shapeDef.density = m_density;

		b2BodyId bodyId = Internal::MakeBodyId(
			rigidBody->getBodyIndex(),
			rigidBody->getBodyWorld0(),
			rigidBody->getBodyGeneration()
		);

		b2ShapeId id = b2CreatePolygonShape(bodyId, &shapeDef, &box);
		m_shapeIndex = id.index1;
		m_shapeWorld0 = id.world0;
		m_shapeGeneration = id.generation;
		m_shapeValid = true;
	}

	void BoxCollider2D::onDetach() {
		Component::onDetach();

		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2DestroyShape(shapeId, true);
			}
			m_shapeValid = false;
		}
	}

	void BoxCollider2D::onDestroy() {
		// onDetach 已处理 shape 销毁，此处仅做保险清理
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2DestroyShape(shapeId, true);
			}
			m_shapeValid = false;
		}
		Component::onDestroy();
	}

	void BoxCollider2D::setSize(const Vector2& size) {
		m_size = { std::max(1.0f, size.x), std::max(1.0f, size.y) };
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Polygon box = b2MakeBox(m_size.x * 0.5f, m_size.y * 0.5f);
				b2Shape_SetPolygon(shapeId, &box);
			}
		}
	}

	void BoxCollider2D::setDensity(float density) {
		m_density = density;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetDensity(shapeId, m_density, true);
			}
		}
	}

	void BoxCollider2D::setFriction(float friction) {
		m_friction = friction;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetFriction(shapeId, m_friction);
			}
		}
	}

	void BoxCollider2D::setRestitution(float restitution) {
		m_restitution = restitution;
		if (m_shapeValid) {
			b2ShapeId shapeId = Internal::MakeShapeId(m_shapeIndex, m_shapeWorld0, m_shapeGeneration);
			if (b2Shape_IsValid(shapeId)) {
				b2Shape_SetRestitution(shapeId, m_restitution);
			}
		}
	}
}