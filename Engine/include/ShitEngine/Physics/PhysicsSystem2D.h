#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../System/System.h"
#include <cstdint>
#include <vector>

namespace Shit {
	class RigidBody2D;

	/**
	 * @brief 2D 物理系统
	 *
	 * 包装 Box2D 3.x（C 语言 API）物理世界。
	 * 默认使用 b2SetLengthUnitsPerMeter(32) 以像素为长度单位，
	 * 用户无需手动转换像素↔米。
	 *
	 * 使用方式：
	 *   scene->registerSystem<Shit::PhysicsSystem2D>();
	 *
	 * priority=50，晚于 BehaviorSystem(0) 早于 RenderSystem(100)。
	 */
	class SHIT_API PhysicsSystem2D final : public System {
	public:
		explicit PhysicsSystem2D(int priority = 50);
		~PhysicsSystem2D() override;

		void init() override;
		void update() override;
		void destroy() override;

		// --- 配置（必须在 init() 之前调用） ---
		void setGravity(const Vector2& gravity);       ///< 重力（像素/秒²），默认 {0, 320}
		const Vector2& getGravity() const { return m_gravity; }

		/// @brief 设置像素↔米比例（必须在 init() 之前调用），默认 32
		void setPixelsPerMeter(float ppm);
		static float GetPixelsPerMeter() { return s_pixelsPerMeter; }

		// --- 供 RigidBody2D 内部调用 ---
		void registerRigidBody(RigidBody2D* body);
		void unregisterRigidBody(RigidBody2D* body);

	private:
		friend class RigidBody2D;

		Vector2 m_gravity{ 0.0f, 320.0f };
		std::vector<RigidBody2D*> m_bodies;

		// b2WorldId = {uint16_t index1; uint16_t generation;}
		uint16_t m_worldIndex = 0;
		uint16_t m_worldGeneration = 0;
		bool m_initialized = false;

		static float s_pixelsPerMeter;
	};
}
