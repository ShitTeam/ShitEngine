#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Physics/Joint2D.h"
#include "ShitEngine/Physics/BoxCollider2D.h"
#include "ShitEngine/Physics/CircleCollider2D.h"

#include <cmath>
#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"

#include "PhysicsInternal.h"

namespace Shit {

	PhysicsSystem2D::PhysicsSystem2D(int priority) : System(priority) {
		m_worldIndex = 0;
		m_worldGeneration = 0;
	}

PhysicsSystem2D::~PhysicsSystem2D() = default;

		void PhysicsSystem2D::onFieldChanged(const std::string& fieldName) {
			// 空字符串 = 所有字段已变更（反序列化后统一同步）
			if (fieldName.empty() || fieldName == "m_gravity") {
				setGravity(m_gravity);  // 即时同步到 Box2D 世界
			}
			if (fieldName.empty() || fieldName == "m_pixelsPerMeter") {
				setPixelsPerMeter(m_pixelsPerMeter);  // 下次 init 生效
			}
		}

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

		// 补建物理体：认领时 Transform 尚未挂载的刚体（.scene 组件顺序不定）在就绪后
		// 创建——仅当 Transform 已存在才重试，避免永久缺 Transform 的对象每帧告警
		for (auto* body : m_bodies) {
			if (!body || body->m_bodyValid) continue;
			auto* owner = body->getOwner();
			if (owner && owner->getComponent<TransformComponent>()) {
				createRigidBody(body);
			}
		}

		// 补建关节：目标刚体（bodyA/bodyB）就绪后才创建（.scene 组件顺序不定/引用晚赋值）
		for (auto* joint : m_joints) {
			if (!joint || joint->m_jointValid) continue;
			createJoint(joint);
		}

		// 同步对象失活状态到物理体：失活对象的刚体移出模拟（不步进、不产生接触），
		// 重新启用后恢复。Enable/Disable 开销大，仅在状态实际变化时调用
		for (auto* body : m_bodies) {
			if (!body || !body->m_bodyValid) continue;
			auto* owner = body->getOwner();
			if (!owner) continue;
			const bool wantEnabled = owner->isActiveInHierarchy();
			b2BodyId bodyId = Internal::MakeBodyId(body->m_bodyIndex, body->m_bodyWorld0, body->m_bodyGeneration);
			if (b2Body_IsEnabled(bodyId) == wantEnabled) continue;
			if (wantEnabled) b2Body_Enable(bodyId);
			else            b2Body_Disable(bodyId);
		}

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

		// --- 接触事件 → 对象级碰撞回调（onCollisionEnter/Stay/Exit） ---
		// Box2D 在步进时缓冲 Begin/End 接触事件（形状级）。语义：
		//   Enter — 本帧新开始的接触（集合中不存在）
		//   Stay  — 上一帧已在集合、本帧仍在集合的对（每帧一次）
		//   Exit  — 本帧结束的接触（end 事件；形状可能已销毁导致解析失败，
		//            由 destroyRigidBody / 碰撞体卸下时的清理兜底）
		b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);

		std::unordered_set<ContactPair, ContactPairHash> prevActive = m_activeContacts; // 本帧处理前集合（上一帧状态）
		std::vector<ContactPair> entered;
		std::vector<ContactPair> exited;
		for (int i = 0; i < contactEvents.beginCount; ++i) {
			const b2ContactBeginTouchEvent& ev = contactEvents.beginEvents[i];
			if (!b2Shape_IsValid(ev.shapeIdA) || !b2Shape_IsValid(ev.shapeIdB)) continue;
			b2BodyId bodyA = b2Shape_GetBody(ev.shapeIdA);
			b2BodyId bodyB = b2Shape_GetBody(ev.shapeIdB);
			RigidBody2D* a = findRigidBody(bodyA.index1, bodyA.world0, bodyA.generation);
			RigidBody2D* b = findRigidBody(bodyB.index1, bodyB.world0, bodyB.generation);
			if (!a || !b) continue;
			ContactPair pair = makeContactPair(a, b);
			if (m_activeContacts.insert(pair).second) {
				entered.push_back(pair);
			}
			// 已在集合中（接触重建/休眠唤醒）：不重复 Enter，Stay 语义已覆盖
		}
		for (int i = 0; i < contactEvents.endCount; ++i) {
			const b2ContactEndTouchEvent& ev = contactEvents.endEvents[i];
			if (!b2Shape_IsValid(ev.shapeIdA) || !b2Shape_IsValid(ev.shapeIdB)) continue;
			b2BodyId bodyA = b2Shape_GetBody(ev.shapeIdA);
			b2BodyId bodyB = b2Shape_GetBody(ev.shapeIdB);
			RigidBody2D* a = findRigidBody(bodyA.index1, bodyA.world0, bodyA.generation);
			RigidBody2D* b = findRigidBody(bodyB.index1, bodyB.world0, bodyB.generation);
			if (!a || !b) continue;
			ContactPair pair = makeContactPair(a, b);
			if (m_activeContacts.erase(pair)) {
				exited.push_back(pair);
			}
		}

		// Stay：上一帧在集合、本帧事件处理后仍在集合的对
		std::vector<ContactPair> stayed;
		for (const ContactPair& pair : m_activeContacts) {
			if (prevActive.count(pair)) stayed.push_back(pair);
		}

		// 派发（先 Enter 后 Stay 再 Exit）。回调可能销毁对象（播放态走延时删除，
		// 编辑态立即删除）——派发函数内逐对校验刚体仍注册、对象仍在场景。
		for (const ContactPair& pair : entered) dispatchContact(pair, CollisionPhase::Enter);
		for (const ContactPair& pair : stayed) dispatchContact(pair, CollisionPhase::Stay);
		for (const ContactPair& pair : exited) dispatchContact(pair, CollisionPhase::Exit);
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
			m_activeContacts.clear();

			// World 销毁使所有关节失效：重置组件标志（不逐个 b2DestroyJoint，
			// World 销毁已回收全部关节对象）
			for (auto* joint : m_joints) {
				if (joint) {
					joint->m_jointValid = false;
					resetComponent(joint);
				}
			}
			m_joints.clear();
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
		if (m_initialized && ppm != m_pixelsPerMeter) {
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
			// 先注册再建体：组件认领时即使 Transform 尚未挂载（.scene 反序列化
			// 顺序不定）也纳入 m_bodies，由 update() 的补建循环在 Transform 就绪后创建
			registerRigidBody(body);
			createRigidBody(body);
			return true;
		}
		if (auto* joint = dynamic_cast<Joint2D*>(component)) {
			// 先注册再建关节：目标刚体可能尚未就绪，由 update() 的补建循环创建
			registerJoint(joint);
			createJoint(joint);
			return true;
		}
		return false;
	}

	void PhysicsSystem2D::onComponentDetached(Component* component) {
		if (auto* body = dynamic_cast<RigidBody2D*>(component)) {
			destroyRigidBody(body);
			return;
		}
		if (auto* joint = dynamic_cast<Joint2D*>(component)) {
			destroyJoint(joint);
			return;
		}
		// 碰撞体卸下：其形状销毁产生的 End 事件可能无法解析（shape 已失效），
		// 清空该刚体的全部接触对——下一帧物理重建接触时重新发 Enter，不留残留。
		if (dynamic_cast<BoxCollider2D*>(component) || dynamic_cast<CircleCollider2D*>(component)) {
			auto* owner = component->getOwner();
			if (auto* body = owner ? owner->getComponent<RigidBody2D>() : nullptr) {
				cleanupContactPairs(body);
			}
		}
	}

	void PhysicsSystem2D::createRigidBody(RigidBody2D* body) {
		if (!body || body->m_bodyValid) return;

		auto* owner = body->getOwner();
		auto* transform = owner ? owner->getComponent<TransformComponent>() : nullptr;
		if (!transform) {
			// 正常瞬态：.scene 反序列化组件顺序不定（Transform 在刚体之后），
			// update() 补建循环/物理 API ensureBody 会在 Transform 就绪后自动创建——
			// 此处降为 DEBUG 避免每次加载刷告警
			ST_CORE_DEBUG("[PhysicsSystem2D] RigidBody2D 暂缺 TransformComponent，等待补建");
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
		// 刚体销毁前清接触对：其后 End 事件 shape 已失效无法自行解析
		cleanupContactPairs(body);
		unregisterRigidBody(body);
		if (body->m_bodyValid) {
			b2BodyId bodyId = Internal::MakeBodyId(body->m_bodyIndex, body->m_bodyWorld0, body->m_bodyGeneration);
			if (b2Body_IsValid(bodyId)) {
				b2DestroyBody(bodyId);
			}
			body->m_bodyValid = false;
		}
	}

	// ═══════════════════════════════════════════════════════════
	// 关节（Joint2D）管理
	// ═══════════════════════════════════════════════════════════

	void PhysicsSystem2D::registerJoint(Joint2D* joint) {
		if (!joint) return;
		if (std::find(m_joints.begin(), m_joints.end(), joint) == m_joints.end()) {
			m_joints.push_back(joint);
		}
	}

	void PhysicsSystem2D::unregisterJoint(Joint2D* joint) {
		if (!joint) return;
		m_joints.erase(
			std::remove(m_joints.begin(), m_joints.end(), joint),
			m_joints.end()
		);
	}

	void PhysicsSystem2D::destroyJoint(Joint2D* joint) {
		if (!joint) return;
		unregisterJoint(joint);
		if (joint->m_jointValid) {
			b2JointId jointId = Internal::MakeJointId(joint->m_jointIndex, joint->m_jointWorld0, joint->m_jointGeneration);
			if (b2Joint_IsValid(jointId)) {
				b2DestroyJoint(jointId);
			}
			joint->m_jointValid = false;
		}
	}

	void PhysicsSystem2D::rebuildJoint(Joint2D* joint) {
		if (!joint) return;
		// 只销毁 Box2D 关节，保留组件注册（m_joints）：创建失败（目标缺失）时
		// 补建循环仍能重试，避免"目标刚体晚就绪"后关节永久丢失
		if (joint->m_jointValid) {
			b2JointId jointId = Internal::MakeJointId(joint->m_jointIndex, joint->m_jointWorld0, joint->m_jointGeneration);
			if (b2Joint_IsValid(jointId)) {
				b2DestroyJoint(jointId);
			}
			joint->m_jointValid = false;
		}
		createJoint(joint);
	}

	void PhysicsSystem2D::createJoint(Joint2D* joint) {
		if (!joint || joint->m_jointValid) return;

		auto* owner = joint->getOwner();
		if (!owner) return;
		RigidBody2D* bodyA = owner->getComponent<RigidBody2D>();
		if (!bodyA || !bodyA->hasValidBody()) return;   // bodyA 未就绪，等补建

		RigidBody2D* bodyB = joint->getConnectedBody();
		if (!bodyB || !bodyB->hasValidBody()) return;    // bodyB 未就绪/未指定，等补建

		b2WorldId worldId = Internal::MakeWorldId(m_worldIndex, m_worldGeneration);
		if (!b2World_IsValid(worldId)) {
			ST_CORE_ERROR("[PhysicsSystem2D] 物理世界无效，无法创建关节");
			return;
		}

		// bodyA 是关节挂载对象的刚体（anchor 默认为 bodyA 位置）
		Vector2 anchor = joint->m_anchor;
		// 默认锚点 = bodyA 当前世界位置（用户未显式设置时）
		b2BodyId bodyIdA = Internal::MakeBodyId(bodyA->m_bodyIndex, bodyA->m_bodyWorld0, bodyA->m_bodyGeneration);
		b2BodyId bodyIdB = Internal::MakeBodyId(bodyB->m_bodyIndex, bodyB->m_bodyWorld0, bodyB->m_bodyGeneration);

		b2Vec2 anchorA = b2Body_GetLocalPoint(bodyIdA, { anchor.x, anchor.y });
		b2Vec2 anchorB = b2Body_GetLocalPoint(bodyIdB, { anchor.x, anchor.y });

		b2JointId result = b2_nullJointId;

		switch (joint->m_type) {
			case JointType::Distance: {
				b2DistanceJointDef def = b2DefaultDistanceJointDef();
				def.bodyIdA = bodyIdA;
				def.bodyIdB = bodyIdB;
				def.localAnchorA = anchorA;
				def.localAnchorB = anchorB;
				float len = joint->m_length;
				if (len <= 0.0f) {
					// 未显式设置自然长度：取两锚点当前世界距离
					b2Vec2 wa = b2Body_GetWorldPoint(bodyIdA, anchorA);
					b2Vec2 wb = b2Body_GetWorldPoint(bodyIdB, anchorB);
					float dx = wb.x - wa.x, dy = wb.y - wa.y;
					len = std::sqrt(dx * dx + dy * dy);
				}
				def.length = len;
				def.enableSpring = joint->m_enableSpring;
				def.hertz = joint->m_hertz;
				def.dampingRatio = joint->m_dampingRatio;
				def.collideConnected = true;
				result = b2CreateDistanceJoint(worldId, &def);
				break;
			}
			case JointType::Revolute: {
				b2RevoluteJointDef def = b2DefaultRevoluteJointDef();
				def.bodyIdA = bodyIdA;
				def.bodyIdB = bodyIdB;
				def.localAnchorA = anchorA;
				def.localAnchorB = anchorB;
				def.collideConnected = true;
				def.enableMotor = joint->m_enableMotor;
				def.motorSpeed = glm::radians(joint->m_motorSpeed);   // 度/秒 → 弧度/秒
				def.maxMotorTorque = joint->m_maxMotorTorque;
				def.enableLimit = joint->m_enableLimit;
				def.lowerAngle = glm::radians(joint->m_lowerAngle);
				def.upperAngle = glm::radians(joint->m_upperAngle);
				result = b2CreateRevoluteJoint(worldId, &def);
				break;
			}
			case JointType::Weld: {
				b2WeldJointDef def = b2DefaultWeldJointDef();
				def.bodyIdA = bodyIdA;
				def.bodyIdB = bodyIdB;
				def.localAnchorA = anchorA;
				def.localAnchorB = anchorB;
				def.collideConnected = true;
				result = b2CreateWeldJoint(worldId, &def);
				break;
			}
			case JointType::Prismatic: {
				b2PrismaticJointDef def = b2DefaultPrismaticJointDef();
				def.bodyIdA = bodyIdA;
				def.bodyIdB = bodyIdB;
				def.localAnchorA = anchorA;
				def.localAnchorB = anchorB;
				// 滑动轴：相对 bodyA 局部坐标，由角度换算单位向量
				float rad = glm::radians(joint->m_axisAngle);
				def.localAxisA = { std::cos(rad), std::sin(rad) };
				def.collideConnected = true;
				def.enableLimit = joint->m_enableLimit;
				def.lowerTranslation = joint->m_lowerTranslation;
				def.upperTranslation = joint->m_upperTranslation;
				def.enableMotor = joint->m_enableMotor;
				def.motorSpeed = joint->m_motorSpeed;                  // 棱柱电机速度单位已是像素/秒
				def.maxMotorForce = joint->m_maxMotorForce;
				result = b2CreatePrismaticJoint(worldId, &def);
				break;
			}
		}

		if (B2_IS_NON_NULL(result)) {
			joint->m_jointIndex = result.index1;
			joint->m_jointWorld0 = result.world0;
			joint->m_jointGeneration = result.generation;
			joint->m_jointValid = true;
			ST_CORE_DEBUG("[PhysicsSystem2D] 已创建关节 {} ({} ↔ {})",
				static_cast<int>(joint->m_type), bodyA->getOwner()->getName(), bodyB->getOwner()->getName());
		}
	}

	PhysicsSystem2D::ContactPair PhysicsSystem2D::makeContactPair(const RigidBody2D* a, const RigidBody2D* b) {
		if (reinterpret_cast<uintptr_t>(a) < reinterpret_cast<uintptr_t>(b)) {
			return { a, b };
		}
		return { b, a };
	}

	RigidBody2D* PhysicsSystem2D::findRigidBody(int32_t bodyIndex, uint16_t world0, uint16_t generation) const {
		for (auto* body : m_bodies) {
			if (body && body->m_bodyValid
					&& body->m_bodyIndex == bodyIndex
					&& body->m_bodyWorld0 == world0
					&& body->m_bodyGeneration == generation) {
				return body;
			}
		}
		return nullptr;
	}

	void PhysicsSystem2D::cleanupContactPairs(const RigidBody2D* body) {
		for (auto it = m_activeContacts.begin(); it != m_activeContacts.end();) {
			if (it->a == body || it->b == body) {
				it = m_activeContacts.erase(it);
			} else {
				++it;
			}
		}
	}

	void PhysicsSystem2D::dispatchContact(const ContactPair& pair, CollisionPhase phase) {
		// 刚体仍注册（指针仅作身份比较，绝不解引用）——销毁中的对象不派发
		if (std::find(m_bodies.begin(), m_bodies.end(), pair.a) == m_bodies.end()) return;
		if (std::find(m_bodies.begin(), m_bodies.end(), pair.b) == m_bodies.end()) return;

		GameObject* ga = pair.a->getOwner();
		GameObject* gb = pair.b->getOwner();
		if (!ga || !gb) return;
		// 对象仍在场景：播放态回调内销毁对象走延时路径，下一对派发前校验兜底
		Scene* scene = getScene();
		if (!scene || !scene->containsGameObject(ga) || !scene->containsGameObject(gb)) return;

		invokeCollisionCallbacks(ga, gb, phase);
		invokeCollisionCallbacks(gb, ga, phase);
	}

	void PhysicsSystem2D::invokeCollisionCallbacks(GameObject* self, GameObject* other, CollisionPhase phase) {
		Scene* scene = getScene();
		if (!scene) return;

		// self 上可能挂多个 Behavior；回调可能销毁 self / other / 移除组件（播放态延时、
		// 编辑态立即）。逐轮重扫：每轮取首个未调用过的已启动行为，调用前校验双方对象存活。
		std::vector<Behavior*> visited; // 仅作身份比较，解除引用
		while (scene->containsGameObject(self) && scene->containsGameObject(other)) {
			Behavior* target = nullptr;
			self->forEachComponent([&](Component* comp) {
				if (target) return;
				auto* behavior = dynamic_cast<Behavior*>(comp);
				if (behavior && behavior->isStarted()
						&& std::find(visited.begin(), visited.end(), behavior) == visited.end()) {
					target = behavior;
				}
			});
			if (!target) break;
			visited.push_back(target);
			switch (phase) {
				case CollisionPhase::Enter: target->onCollisionEnter(other); break;
				case CollisionPhase::Stay:  target->onCollisionStay(other); break;
				case CollisionPhase::Exit:  target->onCollisionExit(other); break;
			}
		}
	}
}