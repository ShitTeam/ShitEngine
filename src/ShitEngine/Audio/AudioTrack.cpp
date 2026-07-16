#include "ShitEngine/Audio/AudioTrack.h"

#include "ShitEngine/Audio/AudioPlayer.h"  // friend 访问 m_group / applyTrackGain
#include <SDL3_mixer/SDL_mixer.h>

namespace Shit {
    AudioTrack::AudioTrack(AudioTrack&& other) noexcept
        : m_handle(other.m_handle)
        , m_gain(other.m_gain)
        , m_loops(other.m_loops)
        , m_group(other.m_group)
        , m_started(other.m_started)
        , m_paused(other.m_paused)
    {
        // other 放弃 handle 后注销自己，再把“自己”登记进同一组，
        // 使组列表指针始终指向当前持有 handle 的 AudioTrack，避免悬空。
        AudioTrackGroup* grp = other.m_group;  // 保存指针：unregister 会清空 other.m_group
        if (grp) {
            grp->unregisterTrack(&other);  // 移除 &other，置空 other.m_group
            grp->registerTrack(this);      // 把 this 加入组并置 this->m_group = grp
        }
        other.m_handle = nullptr;
        other.m_started = false;
        other.m_paused = false;
    }

    AudioTrack& AudioTrack::operator=(AudioTrack&& other) noexcept {
        if (this != &other) {
            // 保存 other 所在组：注销会清空 other.m_group，需先取
            AudioTrackGroup* otherGroup = other.m_group;

            // 把“自己”从原属组注销（this 即将被覆盖，悬空会损坏组列表）
            if (m_group) m_group->unregisterTrack(this);  // 会清空 this->m_group
            if (m_handle) MIX_DestroyTrack(m_handle);

            // other 让出 handle，并从其原属组注销（移除 &other 指针）
            if (otherGroup) otherGroup->unregisterTrack(&other);  // 清空 other.m_group

            m_handle = other.m_handle;
            m_gain = other.m_gain;
            m_loops = other.m_loops;
            m_started = other.m_started;
            m_paused = other.m_paused;

            // 接管后把“自己”登记进原 other 所在组，使组列表指向当前持有者
            if (otherGroup) otherGroup->registerTrack(this);  // 置 m_group = otherGroup
            else m_group = nullptr;

            other.m_handle = nullptr;
            other.m_group = nullptr;
            other.m_started = false;
            other.m_paused = false;
        }
        return *this;
    }

    AudioTrack::~AudioTrack() {
        if (m_handle) MIX_DestroyTrack(m_handle);
    }

    AudioTrack::AudioTrack(MIX_Track* handle) : m_handle(handle), m_started(true) {}

    void AudioTrack::pause() {
        if (!m_handle) return;
        if (MIX_PauseTrack(m_handle)) {
            m_paused = true;
        }
    }

    void AudioTrack::resume() {
        if (!m_handle) return;
        if (MIX_ResumeTrack(m_handle)) {
            m_paused = false;
        }
    }

    void AudioTrack::stop() {
        if (!m_handle) return;
        MIX_DestroyTrack(m_handle);
        m_handle = nullptr;
        m_started = false;
        m_paused = false;
    }

    void AudioTrack::setVolume(float gain) {
        // 仅记录 track 自身系数；实际硬件增益由 AudioPlayer 统一计算下发
        m_gain = gain;
        if (m_group) {
            AudioPlayer::ApplyTrackGain(this, m_group);
        } else {
            AudioPlayer::ApplyTrackGain(this, nullptr);
        }
    }

    float AudioTrack::getVolume() const { return m_gain; }

    void AudioTrack::setLooping(int loopCount) {
        m_loops = loopCount;
        if (m_handle) MIX_SetTrackLoops(m_handle, loopCount);
    }

    bool AudioTrack::isPlaying() const { return m_started && !isFinished(); }

    bool AudioTrack::isPaused() const { return m_paused; }

    bool AudioTrack::isFinished() const {
        if (!m_handle) return true;
        if (!m_started) return false;
        // MIX_GetTrackRemaining: -1 表示时长未知，视为未完成；0 表示已停止
        const Sint64 remaining = MIX_GetTrackRemaining(m_handle);
        return remaining == 0;
    }
}
