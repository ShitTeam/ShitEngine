#pragma once

#include "Component.h"
#include "../Core/Core.h"

namespace Shit
{
    class Animation;

    class SHIT_API AnimationComponent : public Component {
    public:
        AnimationComponent();
        ~AnimationComponent() override = default;

        // 生命周期
        void onAttach() override;
        void onDestroy() override;

        // 添加动画
        void addAnimation(const std::string& name, std::unique_ptr<Animation> animation);

        void play(const std::string& name);
        void stop();
        void pause();
        void resume();
    
    private:
        std::unordered_map<std::string, std::unique_ptr<Animation>> m_animations; // 动画
        Animation* m_currentAnimation = nullptr; // 当前动画
        std::string m_currentAnimationName; // 当前动画名称
        float m_currentTime = 0.0f; // 当前播放时间

        bool m_isPlaying = false;
        bool m_isPaused = false;
    };
}
