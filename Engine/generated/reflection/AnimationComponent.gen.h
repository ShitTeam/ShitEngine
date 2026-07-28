#pragma once

#include <cstddef>
#include <ShitEngine/Component/AnimationComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_AnimationComponent() {
    Shit::ReflectType("AnimationComponent", sizeof(AnimationComponent))
        .Base("Behavior")
        .Field("m_currentAnimationName",
            &Shit::AnimationComponent::m_currentAnimationName, "std::string")
        .Meta(FieldMeta{.displayName = "Current Animation", .readOnly = true})
        .Field("m_currentTime",
            &Shit::AnimationComponent::m_currentTime, "float")
        .Meta(FieldMeta{.displayName = "Current Time", .readOnly = true})
        .Field("m_isPlaying",
            &Shit::AnimationComponent::m_isPlaying, "bool")
        .Meta(FieldMeta{.displayName = "Playing", .readOnly = true})
        .Field("m_isPaused",
            &Shit::AnimationComponent::m_isPaused, "bool")
        .Meta(FieldMeta{.displayName = "Paused", .readOnly = true})
        .Factory<AnimationComponent>()
        .Register<AnimationComponent>();
    return true;
}

} // namespace Shit
