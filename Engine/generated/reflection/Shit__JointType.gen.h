#pragma once

#include <ShitEngine/Physics/Joint2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_JointType() {
    Shit::ReflectType("JointType", sizeof(JointType))
        .Value("Distance", 0)
        .Value("Revolute", 1)
        .Value("Weld", 2)
        .Value("Prismatic", 3)
        .Register<JointType>();
    return true;
}

} // namespace Shit
