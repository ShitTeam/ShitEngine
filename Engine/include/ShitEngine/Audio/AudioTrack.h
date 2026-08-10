#pragma once
#include "../Core/Core.h"
#include <memory>

struct MIX_Track;

namespace Shit {

class AudioTrackGroup;

// SHIT_API：AudioPlayer 在引擎外被按值持有（EngineContext），
// 析构/移动需要跨 DLL 导出符号
class SHIT_API AudioTrack {
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
    // 组生命周期令牌：本 track 仍存活期间持有组引用，组对象不会先于 track 销毁，
    // 析构时 m_group->unregisterTrack(this) 不会访问已释放的组内存
    std::shared_ptr<AudioTrackGroup> m_groupOwner;
    bool m_started = false;
    bool m_paused = false;
};

} // namespace Shit
