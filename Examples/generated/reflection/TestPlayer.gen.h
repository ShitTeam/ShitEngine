#pragma once

#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestPlayer() {
    Shit::ReflectType("TestPlayer", sizeof(TestPlayer))
        .Field("m_hp",
            &TestPlayer::m_hp, "int")
        .Meta(FieldMeta{.displayName = "HP", .tooltip = "Player hit points", .range = {0, 9999}})
        .Field("m_speed",
            &TestPlayer::m_speed, "float")
        .Meta(FieldMeta{.displayName = "Move Speed", .range = {0, 20}, .step = 0.5, .unit = "m/s"})
        .Field("m_name",
            &TestPlayer::m_name, "std::string")
        .Factory<TestPlayer>()
        .Register<TestPlayer>();
    return true;
}

