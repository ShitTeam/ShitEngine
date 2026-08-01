#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Physics/BoxCollider2D.h"
#include "ShitEngine/Physics/CircleCollider2D.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	PhysicsSystem2D::PhysicsSystem2D(int priority) : System(priority) {
		m_worldIndex = 0;
		m_worldGeneration = 0;
	}

	PhysicsSystem2D::~PhysicsSystem2D() = default;

	void PhysicsSystem2D::init() {
		if (m_initialized) return;

		// 告诉 Box2D 使用像素作为长度单位（必须在创建任何 Box2D 对象之前调用）
		b2SetLengthUnitsPerMeter(m_pixelsPerMeter);

		b2WorldDef def = b2DefaultWorldDef();
		def.gravity = { m_gravity.x, m_gravity.y };

		b2WorldId id = b2CreateWorld(&def);
		m_worldIndex = id.index1;
		m_worldGeneration = id.generation;
		m_initialized = true;

		ST_CORE_INFO("[PhysicsSystem2D] 物理世界已创建，重力 ({}, {})，每米 {} 像素",
			m_gravity.x, m_gravity.y, m_pixelsPerMeter);

		// 世界创建后补扫：若 RigidBody2D 先挂、本系统后注册，其 onAttach 曾因找不到
		// 系统而标记 m_isRegistered=false；这里重扫让它们补建物理体（世界必须已存在）。
		System::init();
	}

	void PhysicsSystem2D::update() {
		if (!m_initialized) return;
		if (Shit::Game::IsPaused()) return;  // 全局暂停：冻结物理模拟

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
			// Transform 的旋转以「度」为单位（与编辑器/SDL 渲染约定一致）
			transform->setRotation(glm::degrees(angle));
		}
	}

	void PhysicsSystem2D::destroy() {
		if (m_initialized) {
			b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
			b2DestroyWorld(worldId);
			m_initialized = false;

			// World 销毁使所有刚体/形状失效：重置组件标志，避免悬垂 b2BodyId /
			// 已注册状态残留（同系统重新注册时 System::init 补扫会重建刚体）
			for (auto* body : m_bodies) {
				if (body) {
					body->m_bodyValid = false;
					resetComponent(body);
				}
			}
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
		m_pixelsPerMeter = ppm;
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

	bool PhysicsSystem2D::onComponentAttached(Component* component) {
		if (auto* body = dynamic_cast<RigidBody2D*>(component)) {
			createRigidBody(body);
			return true;
		}
		return false;
	}

	void PhysicsSystem2D::onComponentDetached(Component* component) {
		if (auto* body = dynamic_cast<RigidBody2D*>(component)) {
			destroyRigidBody(body);
		}
	}

	void PhysicsSystem2D::createRigidBody(RigidBody2D* body) {
		if (!body || body->m_bodyValid) return;

		auto* owner = body->getOwner();
		auto* transform = owner ? owner->getComponent<TransformComponent>() : nullptr;
		if (!transform) {
			ST_CORE_WARN("[PhysicsSystem2D] RigidBody2D 缺少 TransformComponent，无法创建物理体");
			return;
		}

		b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
		if (!b2World_IsValid(worldId)) {
			ST_CORE_ERROR("[PhysicsSystem2D] 物理世界无效");
			return;
		}

		b2BodyDef def = b2DefaultBodyDef();
		def.type = static_cast<b2BodyType>(static_cast<int>(body->m_type));
		// 注：b2SetLengthUnitsPerMeter 已设置，所有位置/速度单位为像素
		def.position = { transform->getPosition().x, transform->getPosition().y };
		// Transform 的旋转以「度」为单位，Box2D 需要弧度
		def.rotation = b2MakeRot(glm::radians(transform->getRotation()));
		def.gravityScale = body->m_gravityScale;
		def.linearDamping = body->m_linearDamping;
		def.fixedRotation = body->m_fixedRotation;

		b2BodyId id = b2CreateBody(worldId, &def);
		body->m_bodyIndex = id.index1;
		body->m_bodyWorld0 = id.world0;
		body->m_bodyGeneration = id.generation;
		body->m_bodyValid = true;

		registerRigidBody(body);

		// 补建碰撞形状：同物体上的碰撞体可能先于刚体挂载（Prefab 反序列化顺序不定、
		// 用户先 addComponent<Collider> 后 addComponent<RigidBody>），刚体创建后需重试。
		if (auto* owner = body->getOwner()) {
			owner->forEachComponent([](Component* comp) {
				if (auto* box = dynamic_cast<BoxCollider2D*>(comp)) box->ensureShape();
				else if (auto* circle = dynamic_cast<CircleCollider2D*>(comp)) circle->ensureShape();
			});
		}
	}

	void PhysicsSystem2D::destroyRigidBody(RigidBody2D* body) {
		if (!body) return;
		unregisterRigidBody(body);
		if (body->m_bodyValid) {
			b2BodyId bodyId = Internal::MakeBodyId(body->m_bodyIndex, body->m_bodyWorld0, body->m_bodyGeneration);
			if (b2Body_IsValid(bodyId)) {
				b2DestroyBody(bodyId);
			}
			body->m_bodyValid = false;
		}
	}
}