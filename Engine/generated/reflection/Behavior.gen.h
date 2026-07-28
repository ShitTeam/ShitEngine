#pragma once

#include <ShitEngine/Component/Behavior.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Behavior() {
    Shit::ReflectType("Behavior", sizeof(Behavior))
        .Base("Component")
        .Field("m_isStarted",
            &Shit::Behavior::m_isStarted, "bool")
        .Meta(FieldMeta{.displayName = "Started", .tooltip = "onStart 是否已执行过", .readOnly = true})
        .Factory<Behavior>()
        .Register<Behavior>();
    return true;
}

} // namespace Shit
