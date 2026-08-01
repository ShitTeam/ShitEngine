#pragma once

#include <cstddef>
#include <ShitEngine/Component/Component.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Component() {
    Shit::ReflectType("Component", sizeof(Component))
        .Field("m_owner",
            &Shit::Component::m_owner, "GameObject *")
        .Meta(Shit::FieldMeta{.displayName = "Owner", .readOnly = true})
        .Field("m_isRegistered",
            &Shit::Component::m_isRegistered, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Registered", .readOnly = true})
        .Factory<Component>()
        .Register<Component>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(Component) == 24,
        "Component: size mismatch - regenerate reflection data");
    static_assert(offsetof(Component, m_owner) == 8,
        "Component::m_owner: offset mismatch - regenerate reflection data");
    static_assert(offsetof(Component, m_isRegistered) == 16,
        "Component::m_isRegistered: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
