#pragma once
#include "../Core/Core.h"

struct MIX_Track;

namespace Shit {

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

    void Play();                     // 开始或重新播放
    void Pause();                    // 暂停
    void Resume();                   // 恢复
    void Stop();                     // 停止并重置
    void SetVolume(float gain);      // 0.0 ~ 1.0
    float GetVolume() const;
    void SetLooping(int loopCount);  // -1 = 无限, 0 = 不循环, N = N 次
    void SetFadeIn(float seconds);

    bool IsPlaying() const;
    bool IsPaused() const;
    bool IsFinished() const;

private:
    friend class AudioPlayer;
    AudioTrack() = default;
    explicit AudioTrack(MIX_Track* handle);

    MIX_Track* m_handle = nullptr;
    float m_gain = 1.0f;
    int m_loops = 0;
};

} // namespace Shit
