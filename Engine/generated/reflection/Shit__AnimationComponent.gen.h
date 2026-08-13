#pragma once

#include <ShitEngine/Component/AnimationComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_AnimationComponent() {
    Shit::ReflectType("AnimationComponent", sizeof(AnimationComponent))
        .Base("Behavior")
        .Field("m_clipsData",
            &Shit::AnimationComponent::m_clipsData, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Animation Clips", .tooltip = "序列化载体，由编辑器维护（JSON）", .readOnly = true})
        .Field("m_currentAnimationName",
            &Shit::AnimationComponent::m_currentAnimationName, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Current Animation", .readOnly = true})
        .Field("m_currentTime",
            &Shit::AnimationComponent::m_currentTime, "float")
        .Meta(Shit::FieldMeta{.displayName = "Current Time", .readOnly = true})
        .Field("m_isPlaying",
            &Shit::AnimationComponent::m_isPlaying, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Playing", .readOnly = true})
        .Field("m_isPaused",
            &Shit::AnimationComponent::m_isPaused, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Paused", .readOnly = true})
        .Factory<AnimationComponent>()
        .Register<AnimationComponent>();
    return true;
}

} // namespace Shit
