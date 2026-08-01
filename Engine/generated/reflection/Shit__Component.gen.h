#pragma once

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
    return true;
}

} // namespace Shit
