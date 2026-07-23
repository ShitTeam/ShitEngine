#pragma once

#include "../Core/Core.h"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>

namespace Shit{
    /**
     * @brief 逐帧动画数据
     *
     * 存储一组帧矩形（SDL_FRect）及每帧持续时间。
     * getFrame(elapsedTime) 根据当前播放时间返回对应帧。
     * 通常由 AnimationComponent 自动管理，不直接使用。
     */
    class SHIT_API Animation final {
    public:
        Animation(float duration = 0.1f, bool loop = true);
        ~Animation();

        void addFrame(const SDL_FRect& frame);      ///< 添加单帧
        void addFrames(const std::vector<SDL_FRect>& frames); ///< 批量添加帧

        SDL_FRect getFrame(float elapsedTime) const; ///< 根据已播放时间返回当前帧的源矩形

        // --- getter & setter ---
        void setLoop(bool loop) { m_loop = loop; }
        void setDuration(float duration) { m_duration = duration; }

        bool isLooping() const { return m_loop; }
        float getDuration() const { return m_duration; }
        int getFrameCount() const { return static_cast<int>(m_frames.size()); }

    private:
        std::vector<SDL_FRect> m_frames;
        float m_duration;
        bool m_loop = true;
    };
}