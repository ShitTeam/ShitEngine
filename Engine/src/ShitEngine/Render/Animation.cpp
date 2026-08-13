#include "ShitEngine/Core/pch.h"
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

    float Animation::getFrameDuration(int i) const {
        if (i >= 0 && i < static_cast<int>(m_frameDurations.size()))
            return m_frameDurations[static_cast<size_t>(i)];
        return m_duration;
    }

    float Animation::getTotalDuration() const {
        if (m_frames.empty()) return 0.0f;
        if (m_frameDurations.size() == m_frames.size()) {
            float total = 0.0f;
            for (float d : m_frameDurations) total += d;
            return total;
        }
        return static_cast<float>(m_frames.size()) * m_duration;
    }

    SDL_FRect Animation::getFrame(float elapsedTime) const
    {
        if (m_frames.empty()) return SDL_FRect{};

        const int frameCount = static_cast<int>(m_frames.size());
        const bool perFrame = (m_frameDurations.size() == static_cast<size_t>(frameCount));

        if (m_loop) {
            const float total = getTotalDuration();
            if (total > 0.0f) elapsedTime = std::fmod(elapsedTime, total);
        }

        // 逐帧时长：按累计时间定位帧
        if (perFrame) {
            if (elapsedTime < 0.0f) elapsedTime = 0.0f;
            float acc = 0.0f;
            for (int i = 0; i < frameCount; ++i) {
                const float d = m_frameDurations[static_cast<size_t>(i)];
                if (d <= 0.0f) continue;
                acc += d;
                if (elapsedTime < acc)
                    return m_frames[static_cast<size_t>(i)];
            }
            return m_frames.back();
        }

        // 统一时长（兼容旧路径）
        if (m_duration <= 0.0f) return m_frames[0];
        int index = static_cast<int>(elapsedTime / m_duration);
        if (index < 0) index = 0;
        if (index >= frameCount) index = frameCount - 1;
        return m_frames[static_cast<size_t>(index)];
    }
}