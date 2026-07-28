#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	float PhysicsSystem2D::s_pixelsPerMeter = 32.0f;

	PhysicsSystem2D::PhysicsSystem2D(int priority) : System(priority) {
		m_worldIndex = 0;
		m_worldGeneration = 0;
	}

	PhysicsSystem2D::~PhysicsSystem2D() = default;

	void PhysicsSystem2D::init() {
		if (m_initialized) return;

		// 告诉 Box2D 使用像素作为长度单位（必须在创建任何 Box2D 对象之前调用）
		b2SetLengthUnitsPerMeter(s_pixelsPerMeter);

		b2WorldDef def = b2DefaultWorldDef();
		def.gravity = { m_gravity.x, m_gravity.y };

		b2WorldId id = b2CreateWorld(&def);
		m_worldIndex = id.index1;
		m_worldGeneration = id.generation;
		m_initialized = true;

		ST_CORE_INFO("[PhysicsSystem2D] 物理世界已创建，重力 ({}, {})，每米 {} 像素",
			m_gravity.x, m_gravity.y, s_pixelsPerMeter);
	}

	void PhysicsSystem2D::update() {
		if (!m_initialized) return;

		b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
		// 使用固定时间步长保证物理稳定性，避免低帧率导致穿透
		constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;
		constexpr int MAX_SUB_STEPS = 3;
		float dt = Shit::Time::GetDeltaTime();
		m_accumulator += dt;
		int steps = 0;
		while (m_accumulator >= FIXED_TIME_STEP && steps < MAX_SUB_STEPS) {
			b2World_Step(worldId, FIXED_TIME_STEP, 4);
			m_accumulator -= FIXED_TIME_STEP;
			++steps;
		}
		// 防止 accumulator 无限增长（如断点调试时）
		if (m_accumulator > FIXED_TIME_STEP * MAX_SUB_STEPS) {
			m_accumulator = FIXED_TIME_STEP * MAX_SUB_STEPS;
		}

		// 将所有 Dynamic 和 Kinematic 刚体的位置/旋转同步到 TransformComponent
		// 注：Static 刚体不受物理引擎驱动，无需同步
		for (auto* body : m_bodies) {
			if (!body || !body->m_bodyValid) continue;
			if (body->m_type == RigidBody2D::Type::Static) continue;

			b2BodyId bodyId = Internal::MakeBodyId(body->m_bodyIndex, body->m_bodyWorld0, body->m_bodyGeneration);

			auto* transform = body->getOwner()->getComponent<TransformComponent>();
			if (!transform) continue;

			b2Vec2 pos = b2Body_GetPosition(bodyId);
			b2Rot rot = b2Body_GetRotation(bodyId);
			float angle = b2Rot_GetAngle(rot);

			transform->setPosition({ pos.x, pos.y });
			transform->setRotation(angle);
		}
	}

	void PhysicsSystem2D::destroy() {
		if (m_initialized) {
			b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
			b2DestroyWorld(worldId);
			m_initialized = false;
			m_bodies.clear();
			ST_CORE_INFO("[PhysicsSystem2D] 物理世界已销毁");
		}
	}

	void PhysicsSystem2D::setGravity(const Vector2& gravity) {
		m_gravity = gravity;
		if (m_initialized) {
			b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
			b2World_SetGravity(worldId, { gravity.x, gravity.y });
		}
	}

	void PhysicsSystem2D::setPixelsPerMeter(float ppm) {
		if (ppm <= 0.0f) {
			ST_CORE_ERROR("[PhysicsSystem2D] pixelsPerMeter ({}) 必须大于 0，保持默认 32.0f", ppm);
			return;
		}
		if (m_initialized) {
			ST_CORE_WARN("[PhysicsSystem2D] pixelsPerMeter 修改将在下次 init() 后生效");
		}
		s_pixelsPerMeter = ppm;
	}

	void PhysicsSystem2D::registerRigidBody(RigidBody2D* body) {
		if (!body) return;
		if (std::find(m_bodies.begin(), m_bodies.end(), body) == m_bodies.end()) {
			m_bodies.push_back(body);
		}
	}

	void PhysicsSystem2D::unregisterRigidBody(RigidBody2D* body) {
		if (!body) return;
		m_bodies.erase(
			std::remove(m_bodies.begin(), m_bodies.end(), body),
			m_bodies.end()
		);
	}
}