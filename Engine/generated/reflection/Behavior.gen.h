#pragma once

#include <cstddef>
#include <ShitEngine/Component/Behavior.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Behavior() {
    Shit::ReflectType("Behavior", sizeof(Behavior))
        .Base("Component")
        .Field("m_isStarted",
            &Shit::Behavior::m_isStarted, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Started", .tooltip = "onStart 是否已执行过", .readOnly = true})
        .Factory<Behavior>()
        .Register<Behavior>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(Behavior) == 24,
        "Behavior: size mismatch - regenerate reflection data");
    static_assert(offsetof(Behavior, m_isStarted) == 17,
        "Behavior::m_isStarted: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
