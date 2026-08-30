#pragma once

#include "../Core/Core.h"
#include "../Math.h"
#include "../Animation/AnimationClip.h"   // AnimFrame
#include <string>
#include <vector>

namespace Shit{
    /**
     * @brief 逐帧动画数据
     *
     * 存储一组帧（AnimFrame：纹理路径 + 源矩形）及每帧持续时间。
     * getFrame(elapsedTime) 根据当前播放时间返回对应帧。
     * 通常由 AnimationComponent 自动管理，不直接使用。
     */
    class SHIT_API Animation final {
    public:
        Animation(float duration = 0.1f, bool loop = true);
        ~Animation();

        void addFrame(const AnimFrame& frame);      ///< 添加单帧（带纹理路径）
        void addFrame(const Rect& rect);            ///< 兼容：空路径（沿用当前纹理，动态 play API 用）
        void addFrames(const std::vector<Rect>& frames); ///< 批量添加（空路径，兼容）

        /// 设置每帧独立时长（秒）。传入空 → 回退到统一 duration；传入长度须等于帧数。
        void setFrameDurations(const std::vector<float>& durations) { m_frameDurations = durations; }
        /// 清除每帧独立时长，回退到统一 duration
        void clearFrameDurations() { m_frameDurations.clear(); }

        AnimFrame getFrame(float elapsedTime) const; ///< 根据已播放时间返回当前帧（纹理+矩形）

        // --- getter & setter ---
        void setLoop(bool loop) { m_loop = loop; }
        void setDuration(float duration) { m_duration = duration; }

        bool isLooping() const { return m_loop; }
        float getDuration() const { return m_duration; }
        /// 总时长（秒）：有逐帧时长则累加，否则 frameCount × duration
        float getTotalDuration() const;
        /// 取第 i 帧的时长（秒）：逐帧存在用逐帧值，否则统一 duration
        float getFrameDuration(int i) const;
        int getFrameCount() const { return static_cast<int>(m_frames.size()); }

    private:
        std::vector<AnimFrame> m_frames;
        std::vector<float> m_frameDurations;   ///< 每帧独立时长（可选；空=统一 m_duration）
        float m_duration;
        bool m_loop = true;
    };
}
