#pragma once

#include <ShitEngine/System/BehaviorSystem.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_BehaviorSystem() {
    Shit::ReflectType("BehaviorSystem", sizeof(BehaviorSystem))
        .Base("System")
        .Factory<BehaviorSystem>()
        .Register<BehaviorSystem>();
    return true;
}

} // namespace Shit
