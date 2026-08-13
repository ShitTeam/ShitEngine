#pragma once

#include <ShitEngine/Physics/PhysicsSystem2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_PhysicsSystem2D() {
    Shit::ReflectType("PhysicsSystem2D", sizeof(PhysicsSystem2D))
        .Base("System")
        .Field("m_gravity",
            &Shit::PhysicsSystem2D::m_gravity, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Gravity", .tooltip = "重力（像素/秒²）"})
        .Field("m_pixelsPerMeter",
            &Shit::PhysicsSystem2D::m_pixelsPerMeter, "float")
        .Meta(Shit::FieldMeta{.displayName = "Pixels Per Meter", .tooltip = "每米对应的像素数（默认 32，下次 init 生效）", .range = {1, 1024}, .step = 1})
        .Factory<PhysicsSystem2D>()
        .Register<PhysicsSystem2D>();
    return true;
}

} // namespace Shit
