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
        other.m_handle = nullptr;
        other.m_group = nullptr;
        other.m_started = false;
        other.m_paused = false;
    }

    AudioTrack& AudioTrack::operator=(AudioTrack&& other) noexcept {
        if (this != &other) {
            if (m_handle) MIX_DestroyTrack(m_handle);
            m_handle = other.m_handle;
            m_gain = other.m_gain;
            m_loops = other.m_loops;
            m_group = other.m_group;
            m_started = other.m_started;
            m_paused = other.m_paused;
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
        // 仅记录 track 自身系数；实际硬件增益由 AudioPlayer::applyTrackGain 统一计算下发
        m_gain = gain;
        if (m_group) {
            AudioPlayer::GetInstance().applyTrackGain(this, m_group);
        } else {
            AudioPlayer::GetInstance().applyTrackGain(this, nullptr);
        }
    }

    float AudioTrack::getVolume() const { return m_gain; }

    void AudioTrack::setLooping(int loopCount) { m_loops = loopCount; }

    void AudioTrack::setFadeIn(float /*seconds*/) {
        // TODO: 淡入未实现
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
