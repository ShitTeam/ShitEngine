// Physics 模块内部共用头文件（仅 .cpp 引用，不对外暴露）

#pragma once
#include <box2d/box2d.h>
#include <cstdint>

namespace Shit::Internal {

	/// @brief 从字段还原 b2WorldId
	inline b2WorldId MakeWorldId(uint16_t index1, uint16_t generation) {
		b2WorldId id = { index1, generation };
		return id;
	}

	/// @brief 从字段还原 b2BodyId
	inline b2BodyId MakeBodyId(int32_t index1, uint16_t world0, uint16_t generation) {
		b2BodyId id = { index1, world0, generation };
		return id;
	}

	/// @brief 从字段还原 b2ShapeId
	inline b2ShapeId MakeShapeId(int32_t index1, uint16_t world0, uint16_t generation) {
		b2ShapeId id = { index1, world0, generation };
		return id;
	}

} // namespace Shit::Internal
