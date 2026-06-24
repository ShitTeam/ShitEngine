#include "ShitEngine/Component/AnimationComponent.h"
#include "ShitEngine/Render/Animation.h"

namespace Shit {
    AnimationComponent::AnimationComponent() = default;

    void AnimationComponent::onAttach() {

    }

    void AnimationComponent::onDestroy() {

    }

    void AnimationComponent::addAnimation(const std::string& animationName, std::unique_ptr<Animation> animation) {
        if (!animation) {
            ST_CORE_WARN("试图添加空动画指针!");
            return;
        }
        m_animations[animationName] = std::move(animation);
    }
}