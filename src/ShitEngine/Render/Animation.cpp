#include "ShitEngine/Render/Animation.h"
#include "Animation.h"

namespace Shit {
    Animation::Animation(float duration = 0.1f, bool loop = true) : m_duration(duration), m_loop(loop) { }
    Animation::~Animation() = default;

    void Animation::addFrame(const sf::IntRect& frame) {
        m_frames.push_back(frame);
    }

    void Animation::addFrames(const std::vector<sf::IntRect> &frames)
    {
        for(const auto& frame : frames){
            m_frames.push_back(frame);
        }
    }

    sf::IntRect Animation::getFrame(float totalTime) const
    {
        if (m_frames.empty()) {
            return sf::IntRect();
        }

        if (m_loop) { // 如果是循环，则把循环的部分删掉
            totalTime = fmod(totalTime, m_frames.size() * m_duration);
        }

        // 获取索引
        int index = totalTime / m_duration;

        return m_frames[index];
    }
}