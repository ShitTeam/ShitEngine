#pragma once

#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestEnemy() {
    Shit::ReflectType("TestEnemy", sizeof(TestEnemy))
        .Field("m_health",
            &TestEnemy::m_health, "float")
        .Field("m_damage",
            &TestEnemy::m_damage, "int")
        .Register<TestEnemy>();
    return true;
}

