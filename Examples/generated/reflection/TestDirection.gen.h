#pragma once

#include <ReflectionTestTypes.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_TestDirection() {
    Shit::ReflectType("TestDirection", sizeof(TestDirection))
        .Value("None", 0)
        .Value("Left", 1)
        .Value("Right", 2)
        .Value("Up", 4)
        .Value("Down", 8)
        .Register<TestDirection>();
    return true;
}

