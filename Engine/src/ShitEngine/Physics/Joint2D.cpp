#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Physics/Joint2D.h"
#include "ShitEngine/Physics/RigidBody2D.h"
#include "ShitEngine/Physics/PhysicsSystem2D.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"

#include "PhysicsInternal.h"

namespace Shit {

	Joint2D::Joint2D() = default;
	Joint2D::~Joint2D() = default;

	void Joint2D::onCreate() {
		Component::onCreate();
	}

	void Joint2D::onAttach() {
		Component::onAttach();

		// 广播给 Scene，由 PhysicsSystem2D 认领并创建 Box2D 关节（解耦）。
		// 系统未注册时 registerComponent 返回 false → 自愈补注册（同 RigidBody2D 语义）。
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			m_isRegistered = scene->registerComponent(this);
			if (!m_isRegistered) {
				scene->registerSystem<PhysicsSystem2D>();
				m_isRegistered = scene->registerComponent(this);
			}
		} else {
			m_isRegistered = false;
		}
	}

	void Joint2D::onDetach() {
		Component::onDetach();

		// 广播卸下，由 PhysicsSystem2D 销毁 Box2D 关节并注销
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			scene->unregisterComponent(this);
		}

		// 兜底：若 Box2D 关节仍有效（系统未处理），直接销毁
		if (m_jointValid) {
			b2JointId jointId = Internal::MakeJointId(m_jointIndex, m_jointWorld0, m_jointGeneration);
			if (b2Joint_IsValid(jointId)) {
				b2DestroyJoint(jointId);
			}
			m_jointValid = false;
		}
	}

	void Joint2D::onDestroy() {
		if (m_jointValid) {
			ST_CORE_WARN("[Joint2D] 组件直接销毁而未 detach");
			auto* scene = m_owner ? m_owner->getScene() : nullptr;
			if (scene) {
				if (auto* physics = scene->getSystem<PhysicsSystem2D>()) {
					physics->unregisterJoint(this);
				}
			}
			b2JointId jointId = Internal::MakeJointId(m_jointIndex, m_jointWorld0, m_jointGeneration);
			if (b2Joint_IsValid(jointId)) {
				b2DestroyJoint(jointId);
			}
			m_jointValid = false;
		}
		Component::onDestroy();
	}

	void Joint2D::onFieldChanged(const std::string& fieldName) {
		// 检查器直写反射字段后：统一重建关节。Box2D 不同关节的运行时参数 setter 不统一，
		// 且类型切换无法原地改；重建（销毁+按当前字段重建）语义清晰、覆盖所有字段。
		// 无体/目标缺失时等系统补建（createJoint 读 m_* 字段）即正确。
		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (!scene) return;
		auto* physics = scene->getSystem<PhysicsSystem2D>();
		if (!physics) return;
		physics->rebuildJoint(this);
		(void)fieldName;
	}

	void Joint2D::setType(JointType type) {
		if (m_type == type) return;
		m_type = type;
		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (scene && scene->getSystem<PhysicsSystem2D>()) {
			scene->getSystem<PhysicsSystem2D>()->rebuildJoint(this);
		}
	}

	void Joint2D::setConnectedBody(RigidBody2D* body) {
		m_connectedBody.setUuid(body ? body->getUuid() : 0);
		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (scene && scene->getSystem<PhysicsSystem2D>()) {
			scene->getSystem<PhysicsSystem2D>()->rebuildJoint(this);
		}
	}

	RigidBody2D* Joint2D::getConnectedBody() const {
		return m_connectedBody.get();
	}

	void Joint2D::setAnchor(const Vector2& anchor) {
		if (m_anchor == anchor) return;
		m_anchor = anchor;
		auto* scene = m_owner ? m_owner->getScene() : nullptr;
		if (scene && scene->getSystem<PhysicsSystem2D>()) {
			scene->getSystem<PhysicsSystem2D>()->rebuildJoint(this);
		}
	}

	void Joint2D::setLength(float length) {
		if (m_length == length) return;
		m_length = length;
		if (m_type == JointType::Distance) onFieldChanged("m_length");
	}

	void Joint2D::setEnableSpring(bool enable) {
		if (m_enableSpring == enable) return;
		m_enableSpring = enable;
		if (m_type == JointType::Distance) onFieldChanged("m_enableSpring");
	}

	void Joint2D::setHertz(float hertz) {
		if (m_hertz == hertz) return;
		m_hertz = hertz;
		if (m_type == JointType::Distance) onFieldChanged("m_hertz");
	}

	void Joint2D::setDampingRatio(float ratio) {
		if (m_dampingRatio == ratio) return;
		m_dampingRatio = ratio;
		if (m_type == JointType::Distance) onFieldChanged("m_dampingRatio");
	}

	void Joint2D::setEnableMotor(bool enable) {
		if (m_enableMotor == enable) return;
		m_enableMotor = enable;
		if (m_type == JointType::Revolute || m_type == JointType::Prismatic) onFieldChanged("m_enableMotor");
	}

	void Joint2D::setMotorSpeed(float speed) {
		if (m_motorSpeed == speed) return;
		m_motorSpeed = speed;
		if (m_type == JointType::Revolute || m_type == JointType::Prismatic) onFieldChanged("m_motorSpeed");
	}

	void Joint2D::setMaxMotorTorque(float torque) {
		if (m_maxMotorTorque == torque) return;
		m_maxMotorTorque = torque;
		if (m_type == JointType::Revolute) onFieldChanged("m_maxMotorTorque");
	}

	void Joint2D::setEnableLimit(bool enable) {
		if (m_enableLimit == enable) return;
		m_enableLimit = enable;
		if (m_type == JointType::Revolute || m_type == JointType::Prismatic) onFieldChanged("m_enableLimit");
	}

	void Joint2D::setLowerAngle(float deg) {
		if (m_lowerAngle == deg) return;
		m_lowerAngle = deg;
		if (m_type == JointType::Revolute) onFieldChanged("m_lowerAngle");
	}

	void Joint2D::setUpperAngle(float deg) {
		if (m_upperAngle == deg) return;
		m_upperAngle = deg;
		if (m_type == JointType::Revolute) onFieldChanged("m_upperAngle");
	}

	void Joint2D::setAxisAngle(float deg) {
		if (m_axisAngle == deg) return;
		m_axisAngle = deg;
		if (m_type == JointType::Prismatic) onFieldChanged("m_axisAngle");
	}

	void Joint2D::setLowerTranslation(float v) {
		if (m_lowerTranslation == v) return;
		m_lowerTranslation = v;
		if (m_type == JointType::Prismatic) onFieldChanged("m_lowerTranslation");
	}

	void Joint2D::setUpperTranslation(float v) {
		if (m_upperTranslation == v) return;
		m_upperTranslation = v;
		if (m_type == JointType::Prismatic) onFieldChanged("m_upperTranslation");
	}

	void Joint2D::setMaxMotorForce(float force) {
		if (m_maxMotorForce == force) return;
		m_maxMotorForce = force;
		if (m_type == JointType::Prismatic) onFieldChanged("m_maxMotorForce");
	}

} // namespace Shit
