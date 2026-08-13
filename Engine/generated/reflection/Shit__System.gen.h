#pragma once

#include <ShitEngine/System/System.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_System() {
    Shit::ReflectType("System", sizeof(System))
        .Factory<System>()
        .Register<System>();
    return true;
}

} // namespace Shit
