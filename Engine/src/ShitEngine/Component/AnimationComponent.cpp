#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/AnimationComponent.h"

#include "ShitEngine/Render/Animation.h"
#include "ShitEngine/Component/SpriteRenderer.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"

#include <utility>

namespace Shit {
    AnimationComponent::AnimationComponent() = default;
    AnimationComponent::~AnimationComponent() = default;

    void AnimationComponent::onAttach() {
        Behavior::onAttach();  // 把自己注册进 BehaviorSystem
    }

    void AnimationComponent::onStart() {
        // 首次启动若已有“当前动画”则把首帧应用上去
        if (m_currentAnimation) applyCurrentFrame();
    }

    void AnimationComponent::onUpdate() {
        if (!m_isPlaying || m_isPaused || !m_currentAnimation) return;

        // 非循环动画播到末尾：停在最后一帧并结束播放
        if (!m_currentAnimation->isLooping()) {
            float totalLen = static_cast<float>(m_currentAnimation->getFrameCount())
                             * m_currentAnimation->getDuration();
            if (totalLen > 0.0f && m_currentTime >= totalLen) {
                m_currentTime = totalLen;  // clamp 到末帧
                applyCurrentFrame();
                m_isPlaying = false;
                return;
            }
        }

        m_currentTime += Time::GetDeltaTime();
        applyCurrentFrame();
    }

    void AnimationComponent::onDestroy() {
        Behavior::onDestroy();
        m_currentAnimation = nullptr;
        m_currentAnimationName.clear();
        m_animations.clear();
        m_isPlaying = false;
        m_isPaused = false;
    }

    void AnimationComponent::addAnimation(const std::string& animationName, std::unique_ptr<Animation> animation) {
        if (!animation) {
            ST_CORE_WARN("试图添加空动画指针!");
            return;
        }
        m_animations[animationName] = std::move(animation);
    }

    void AnimationComponent::play(const std::string& name) {
        auto it = m_animations.find(name);
        if (it == m_animations.end() || !it->second) {
            ST_CORE_WARN("找不到名为 {} 的动画！", name);
            return;
        }
        if (m_currentAnimationName != name) {
            m_currentAnimation = it->second.get();
            m_currentAnimationName = name;
            m_currentTime = 0.0f;
        }
        m_isPlaying = true;
        m_isPaused = false;
        applyCurrentFrame();
    }

    void AnimationComponent::play(const std::string& name, const SpriteSheet& sheet,
                                  const std::vector<int>& frames, float duration, bool loop) {
        auto anim = std::make_unique<Animation>(duration, loop);
        for (int idx : frames) {
            anim->addFrame(sheet.getFrameRect(idx));
        }
        // 登记（存在则覆盖）并切换为当前
        m_animations[name] = std::move(anim);
        m_currentAnimation = m_animations[name].get();
        m_currentAnimationName = name;
        m_currentTime = 0.0f;
        m_isPlaying = true;
        m_isPaused = false;
        applyCurrentFrame();
    }

    void AnimationComponent::stop() {
        m_isPlaying = false;
        m_isPaused = false;
        m_currentTime = 0.0f;

        // 恢复 SpriteRenderer 为整图渲染
        if (auto* sprite = getOwner() ? getOwner()->getComponent<SpriteRenderer>() : nullptr) {
            sprite->setSourceRect(std::nullopt);
        }
    }

    void AnimationComponent::pause() {
        m_isPaused = true;
    }

    void AnimationComponent::resume() {
        if (!m_currentAnimation) return;
        m_isPaused = false;
    }

    void AnimationComponent::applyCurrentFrame() {
        if (!m_currentAnimation) return;
        if (m_currentAnimation->getFrameCount() == 0) return;

        // duration <= 0 时 getFrame 会安全返回首帧
        SDL_FRect frame = m_currentAnimation->getFrame(m_currentTime);

        auto* owner = getOwner();
        if (!owner) return;
        auto* sprite = owner->getComponent<SpriteRenderer>();
        if (sprite) sprite->setSourceRect(frame);
    }
}
