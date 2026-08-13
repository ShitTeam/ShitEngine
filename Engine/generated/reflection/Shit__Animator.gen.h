#pragma once

#include <ShitEngine/Animation/Animator.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Animator() {
    Shit::ReflectType("Animator", sizeof(Animator))
        .Base("Behavior")
        .Field("m_animatorData",
            &Shit::Animator::m_animatorData, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Animator Data", .tooltip = "状态机序列化载体（JSON），由编辑器维护"})
        .Field("m_currentStateDisplay",
            &Shit::Animator::m_currentStateDisplay, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Current State", .readOnly = true})
        .Field("m_playingDisplay",
            &Shit::Animator::m_playingDisplay, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Playing", .readOnly = true})
        .Factory<Animator>()
        .Register<Animator>();
    return true;
}

} // namespace Shit
