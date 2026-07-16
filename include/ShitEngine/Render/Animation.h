#pragma once

#include "../Core/Core.h"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>

namespace Shit{
    /**
     * @brief 动画组件
     *
     * 储存每一帧的动画
     */
    class SHIT_API Animation final {
    public:
        Animation(float duration = 0.1f, bool loop = true);
        ~Animation();

        void addFrame(const SDL_FRect& frame);
        void addFrames(const std::vector<SDL_FRect>& frames);

        SDL_FRect getFrame(float elapsedTime) const;

        // --- getter & setter ---
        void setLoop(bool loop) { m_loop = loop; }
        void setDuration(float duration) { m_duration = duration; }

        bool isLooping() const { return m_loop; }
        float getDuration() const { return m_duration; }
        int getFrameCount() const { return static_cast<int>(m_frames.size()); }

    private:
        std::vector<SDL_FRect> m_frames; // 每帧的矩形位置
        float m_duration; // 每帧时间
        bool m_loop = true; // 是否循环播放
    };
}