#pragma once

#include <CoinDemo.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_BallDemo() {
    Shit::ReflectType("BallDemo", sizeof(BallDemo))
        .Base("Behavior")
        .Field("m_speed",
            &BallDemo::m_speed, "float")
        .Meta(Shit::FieldMeta{.displayName = "Speed", .tooltip = "水平初速度（像素/秒）"})
        .Factory<BallDemo>()
        .Register<BallDemo>();
    return true;
}

