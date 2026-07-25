#pragma once

#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestPlayer() {
    Shit::ReflectType("TestPlayer", sizeof(TestPlayer))
        .Field("m_hp",
            &TestPlayer::m_hp, "int")
        .Field("m_speed",
            &TestPlayer::m_speed, "float")
        .Field("m_name",
            &TestPlayer::m_name, "std::string")
        .Register<TestPlayer>();
    return true;
}

