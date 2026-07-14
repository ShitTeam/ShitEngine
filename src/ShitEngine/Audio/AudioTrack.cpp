#include "ShitEngine/Audio/AudioTrack.h"

#include <SDL3_mixer/SDL_mixer.h>

namespace Shit {
    AudioTrack::AudioTrack(AudioTrack&& other) noexcept
        : m_handle(other.m_handle)
        , m_gain(other.m_gain)
        , m_loops(other.m_loops)
    {
        other.m_handle = nullptr;
    }

    AudioTrack& AudioTrack::operator=(AudioTrack&& other) noexcept {
        if (this != &other) {
            if (m_handle) MIX_DestroyTrack(m_handle);
            m_handle = other.m_handle;
            m_gain = other.m_gain;
            m_loops = other.m_loops;
            other.m_handle = nullptr;
        }
        return *this;
    }

    AudioTrack::~AudioTrack() {
        if (m_handle) MIX_DestroyTrack(m_handle);
    }

    AudioTrack::AudioTrack(MIX_Track* handle) : m_handle(handle) {}

    void AudioTrack::play() {}

    void AudioTrack::pause() {
        if (!m_handle) return;
        if (auto* stream = MIX_GetTrackAudioStream(m_handle)) {
            SDL_PauseAudioStreamDevice(stream);
        }
    }

    void AudioTrack::resume() {
        if (!m_handle) return;
        if (auto* stream = MIX_GetTrackAudioStream(m_handle)) {
            SDL_ResumeAudioStreamDevice(stream);
        }
    }

    void AudioTrack::stop() {
        if (!m_handle) return;
        MIX_DestroyTrack(m_handle);
        m_handle = nullptr;
    }

    void AudioTrack::setVolume(float gain) {
        if (!m_handle) return;
        m_gain = gain;
        MIX_SetTrackGain(m_handle, gain);
    }

    float AudioTrack::getVolume() const { return m_gain; }

    void AudioTrack::setLooping(int loopCount) { m_loops = loopCount; }

    void AudioTrack::setFadeIn(float /*seconds*/) {}

    bool AudioTrack::isPlaying() const { return !isFinished(); }

    bool AudioTrack::isPaused() const { return false; }

    bool AudioTrack::isFinished() const {
        if (!m_handle) return true;
        return MIX_GetTrackRemaining(m_handle) <= 0;
    }
}
