#pragma once

#include <Behaviors.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_GravityConfig() {
    Shit::ReflectType("GravityConfig", sizeof(GravityConfig))
        .Base("Behavior")
        .Field("m_gravity",
            &GravityConfig::m_gravity, "Shit::Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Gravity", .tooltip = "物理世界重力（像素/秒²）"})
        .Factory<GravityConfig>()
        .Register<GravityConfig>();
    return true;
}

