#pragma once

#include <cstddef>
#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestEnemy() {
    Shit::ReflectType("TestEnemy", sizeof(TestEnemy))
        .Field("m_health",
            &TestEnemy::m_health, "float")
        .Field("m_damage",
            &TestEnemy::m_damage, "int")
        .Factory<TestEnemy>()
        .Register<TestEnemy>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(TestEnemy) == 12,
        "TestEnemy: size mismatch - regenerate reflection data");
    static_assert(offsetof(TestEnemy, m_health) == 0,
        "TestEnemy::m_health: offset mismatch - regenerate reflection data");
    static_assert(offsetof(TestEnemy, m_damage) == 4,
        "TestEnemy::m_damage: offset mismatch - regenerate reflection data");
    return true;
}

