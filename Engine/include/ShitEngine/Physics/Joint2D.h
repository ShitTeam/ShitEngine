#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../Component/Component.h"
#include "../GameObject/ComponentRef.h"
#include <cstdint>

namespace Shit {
	class PhysicsSystem2D;
	class RigidBody2D;   // 前向声明（ComponentRef 模板与 setter 参数仅需不完整类型）

	/// @brief 关节类型（决定 Box2D 约束行为）
	enum class SHIT_ENUM(JointType) JointType : int {
		Distance   = 0, ///< 距离关节：保持两锚点间固定距离（可做弹簧）
		Revolute   = 1, ///< 旋转/铰链关节：两刚体绕锚点相对旋转
		Weld       = 2, ///< 焊接关节：把两刚体刚性连在一起（可做软体）
		Prismatic  = 3, ///< 棱柱关节：沿局部轴相对滑动（活塞/滑轨）
	};

	/**
	 * @brief 2D 物理关节组件
	 *
	 * 把本 GameObject 上的 RigidBody2D（bodyA）与另一个刚体（bodyB，由
	 * connectedBody 引用字段指定）用 Box2D 关节约束连接。必须挂在一个已挂
	 * RigidBody2D 的 GameObject 上。
	 *
	 * 使用方式：
	 *   auto* joint = go->addComponent<Shit::Joint2D>();
	 *   joint->setType(Shit::JointType::Revolute);
	 *   joint->setConnectedBody(otherBody);   // 或编辑器拖拽引用字段
	 *   joint->setAnchor({ 400, 100 });       // 世界锚点
	 *
	 * 关节生命周期由 PhysicsSystem2D 驱动：onAttach 时注册、Box2D joint 创建，
	 * 目标刚体缺失时每帧补建（自愈），卸下时销毁。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) Joint2D : public Component {
		SHIT_REFLECT_BODY(Joint2D)
	public:
		Joint2D();
		~Joint2D() override;

		void onCreate() override;
		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		/// 检查器直写反射字段后回调：把运行时字段改动同步到已创建的 Box2D 关节
		void onFieldChanged(const std::string& fieldName) override;

		// --- 配置（可在 onAttach 前后调用） ---
		void setType(JointType type);
		JointType getType() const { return m_type; }

		/// @brief 设置另一端刚体（bodyB）。传 nullptr 清空。
		void setConnectedBody(RigidBody2D* body);
		RigidBody2D* getConnectedBody() const;

		/// @brief 世界锚点（像素）。默认取本刚体位置；revolute/weld/distance 共用心智模型。
		void setAnchor(const Vector2& anchor);
		const Vector2& getAnchor() const { return m_anchor; }

		// --- Distance 关节参数 ---
		void setLength(float length);          ///< 自然长度（像素），默认取两锚点当前距离
		float getLength() const { return m_length; }
		void setEnableSpring(bool enable);     ///< 是否作为弹簧（false=刚性距离）
		void setHertz(float hertz);            ///< 弹簧刚度（Hz）
		void setDampingRatio(float ratio);     ///< 弹簧阻尼比

		// --- Revolute 关节参数 ---
		void setEnableMotor(bool enable);
		void setMotorSpeed(float speed);       ///< 目标角速度（度/秒）
		void setMaxMotorTorque(float torque);
		void setEnableLimit(bool enable);
		void setLowerAngle(float deg);         ///< 角度下限（度）
		void setUpperAngle(float deg);         ///< 角度上限（度）

		// --- Prismatic 关节参数 ---
		void setAxisAngle(float deg);          ///< 滑动轴方向（度，相对 bodyA 局部）
		void setLowerTranslation(float v);     ///< 滑动下限（像素）
		void setUpperTranslation(float v);     ///< 滑动上限（像素）
		void setMaxMotorForce(float force);

		// --- 内部 / 供 PhysicsSystem2D 使用 ---
		bool hasValidJoint() const { return m_jointValid; }
		int32_t getJointIndex() const { return m_jointIndex; }
		uint16_t getJointWorld0() const { return m_jointWorld0; }
		uint16_t getJointGeneration() const { return m_jointGeneration; }

	private:
		friend class PhysicsSystem2D;

		// b2JointId = {int32_t index1; uint16_t world0; uint16_t generation;}
		SHIT_META(Disable)
		int32_t m_jointIndex = 0;
		SHIT_META(Disable)
		uint16_t m_jointWorld0 = 0;
		SHIT_META(Disable)
		uint16_t m_jointGeneration = 0;
		SHIT_META(Disable)
		bool m_jointValid = false;

		SHIT_META(({.displayName = "Type", .tooltip = "Distance=距离, Revolute=铰链, Weld=焊接, Prismatic=滑动"}))
		JointType m_type = JointType::Distance;

		SHIT_META(({.displayName = "Connected Body", .tooltip = "另一端刚体（bodyB）；留空则暂不创建"}))
		ComponentRef<RigidBody2D> m_connectedBody;

		SHIT_META(({.displayName = "Anchor", .tooltip = "世界锚点（像素）"}))
		Vector2 m_anchor{ 0.0f, 0.0f };

		// --- Distance 参数 ---
		SHIT_META(({.displayName = "Length", .tooltip = "距离关节自然长度（像素）", .range = {0, 4096}, .step = 1}))
		float m_length = 0.0f;
		SHIT_META(({.displayName = "Spring", .tooltip = "作为弹簧（false=刚性距离）"}))
		bool m_enableSpring = false;
		SHIT_META(({.displayName = "Hertz", .tooltip = "弹簧刚度 (Hz)", .range = {0, 60}, .step = 0.1}))
		float m_hertz = 4.0f;
		SHIT_META(({.displayName = "Damping Ratio", .tooltip = "弹簧阻尼比", .range = {0, 2}, .step = 0.01}))
		float m_dampingRatio = 0.5f;

		// --- Revolute 参数 ---
		SHIT_META(({.displayName = "Motor", .tooltip = "启用电机"}))
		bool m_enableMotor = false;
		SHIT_META(({.displayName = "Motor Speed", .tooltip = "目标角速度（度/秒）", .range = {-720, 720}, .step = 1}))
		float m_motorSpeed = 0.0f;
		SHIT_META(({.displayName = "Max Motor Torque", .tooltip = "最大电机扭矩", .range = {0, 100000}, .step = 1}))
		float m_maxMotorTorque = 0.0f;
		SHIT_META(({.displayName = "Limit", .tooltip = "启用角度限制"}))
		bool m_enableLimit = false;
		SHIT_META(({.displayName = "Lower Angle", .tooltip = "角度下限（度）", .range = {-360, 360}, .step = 1}))
		float m_lowerAngle = -45.0f;
		SHIT_META(({.displayName = "Upper Angle", .tooltip = "角度上限（度）", .range = {-360, 360}, .step = 1}))
		float m_upperAngle = 45.0f;

		// --- Prismatic 参数 ---
		SHIT_META(({.displayName = "Axis Angle", .tooltip = "滑动轴方向（度，相对 bodyA 局部）", .range = {-360, 360}, .step = 1}))
		float m_axisAngle = 0.0f;
		SHIT_META(({.displayName = "Lower Translation", .tooltip = "滑动下限（像素）", .range = {-4096, 4096}, .step = 1}))
		float m_lowerTranslation = -100.0f;
		SHIT_META(({.displayName = "Upper Translation", .tooltip = "滑动上限（像素）", .range = {-4096, 4096}, .step = 1}))
		float m_upperTranslation = 100.0f;
		SHIT_META(({.displayName = "Max Motor Force", .tooltip = "最大电机力", .range = {0, 100000}, .step = 1}))
		float m_maxMotorForce = 0.0f;
	};
}
