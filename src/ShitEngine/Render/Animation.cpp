#include "ShitEngine/Render/Animation.h"

#include <cmath>

namespace Shit {
    Animation::Animation(float duration, bool loop) : m_duration(duration), m_loop(loop) { }
    Animation::~Animation() = default;

    void Animation::addFrame(const SDL_FRect& frame) {
        m_frames.push_back(frame);
    }

    void Animation::addFrames(const std::vector<SDL_FRect>& frames)
    {
        for (const auto& frame : frames) {
            m_frames.push_back(frame);
        }
    }

    SDL_FRect Animation::getFrame(float elapsedTime) const
    {
        if (m_frames.empty() || m_duration <= 0.0f) {
            return m_frames.empty() ? SDL_FRect{} : m_frames[0];
        }

        // 循环模式下对周期取模
        float frameCount = static_cast<float>(m_frames.size());
        if (m_loop) {
            elapsedTime = std::fmod(elapsedTime, frameCount * m_duration);
        }

        // 计算当前帧索引
        int index = static_cast<int>(elapsedTime / m_duration);
        if (index < 0) {
            index = 0;
        }
        if (index >= static_cast<int>(m_frames.size())) {
            index = static_cast<int>(m_frames.size()) - 1;
        }

        return m_frames[index];
    }
}