#pragma once

#include <cstddef>
#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestPlayer() {
    Shit::ReflectType("TestPlayer", sizeof(TestPlayer))
        .Field("m_hp",
            &TestPlayer::m_hp, "int")
        .Meta(Shit::FieldMeta{.displayName = "HP", .tooltip = "Player hit points", .range = {0, 9999}})
        .Field("m_speed",
            &TestPlayer::m_speed, "float")
        .Meta(Shit::FieldMeta{.displayName = "Move Speed", .range = {0, 20}, .step = 0.5, .unit = "m/s"})
        .Field("m_name",
            &TestPlayer::m_name, "std::string")
        .Factory<TestPlayer>()
        .Register<TestPlayer>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(TestPlayer) == 40,
        "TestPlayer: size mismatch - regenerate reflection data");
    static_assert(offsetof(TestPlayer, m_hp) == 0,
        "TestPlayer::m_hp: offset mismatch - regenerate reflection data");
    static_assert(offsetof(TestPlayer, m_speed) == 4,
        "TestPlayer::m_speed: offset mismatch - regenerate reflection data");
    static_assert(offsetof(TestPlayer, m_name) == 8,
        "TestPlayer::m_name: offset mismatch - regenerate reflection data");
    return true;
}

