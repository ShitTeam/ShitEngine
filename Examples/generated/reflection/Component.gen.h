#pragma once

#include <cstddef>
#include <Component.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Component() {
    ReflectType("Component", sizeof(Component))
        .Field("m_owner",
            8, 8, "GameObject *")
        .Field("m_isRegistered",
            16, 1, "bool")
        .Register<Component>();
    return true;
}

} // namespace Shit
