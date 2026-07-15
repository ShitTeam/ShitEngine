#pragma once
#include "../Core/Core.h"

struct MIX_Track;

namespace Shit {

class AudioTrackGroup;

/**
 * @brief 音频轨道，控制单个音频播放
 */
class AudioTrack {
public:
    AudioTrack(const AudioTrack&) = delete;
    AudioTrack& operator=(const AudioTrack&) = delete;
    AudioTrack(AudioTrack&&) noexcept;
    AudioTrack& operator=(AudioTrack&&) noexcept;
    ~AudioTrack();

    void pause();                    // 暂停
    void resume();                   // 恢复
    void stop();                     // 停止并重置
    void setVolume(float gain);      // 设置轨道自身增益系数（默认 1.0）；实际硬件增益 = master × group × gain
    float getVolume() const;
    void setLooping(int loopCount);  // TODO: 循环尚未接入 MIX_PlayTrack options，当前仅记录不生效。 -1 = 无限, 0 = 不循环, N = N 次

    bool isPlaying() const;
    bool isPaused() const;
    bool isFinished() const;

private:
    friend class AudioPlayer;
    friend class AudioTrackGroup;
    AudioTrack() = default;
    explicit AudioTrack(MIX_Track* handle);

    MIX_Track* m_handle = nullptr;
    float m_gain = 1.0f;
    int m_loops = 0;       // TODO: 尚未接入 mixer
    AudioTrackGroup* m_group = nullptr;
    bool m_started = false;
    bool m_paused = false;
};

} // namespace Shit
