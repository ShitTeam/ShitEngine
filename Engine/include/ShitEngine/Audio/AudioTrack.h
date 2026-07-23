#pragma once
#include "../Core/Core.h"

struct MIX_Track;

namespace Shit {

class AudioTrackGroup;

class AudioTrack {
public:
    AudioTrack(const AudioTrack&) = delete;
    AudioTrack& operator=(const AudioTrack&) = delete;
    AudioTrack(AudioTrack&&) noexcept;
    AudioTrack& operator=(AudioTrack&&) noexcept;
    ~AudioTrack();

    void pause();
    void resume();
    void stop();
    void setVolume(float gain);
    float getVolume() const;
    void setLooping(int loopCount);  // -1 = 无限, 0 = 不循环, N = N 次
    int getLooping() const { return m_loops; }

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
    int m_loops = 0;
    AudioTrackGroup* m_group = nullptr;
    bool m_started = false;
    bool m_paused = false;
};

} // namespace Shit
