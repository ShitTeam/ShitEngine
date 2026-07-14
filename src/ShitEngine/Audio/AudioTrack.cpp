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
        if (m_handle) {
            MIX_DestroyTrack(m_handle);
        }
    }

    AudioTrack::AudioTrack(MIX_Track* handle)
        : m_handle(handle) {}

    void AudioTrack::Play() {
        if (!m_handle) return;
        // SDL3_mixer: tracks play immediately on creation, no explicit Play needed
    }

    void AudioTrack::Pause() {
        if (!m_handle) return;
        // Use SDL audio stream to pause
        if (auto* stream = MIX_GetTrackAudioStream(m_handle)) {
            SDL_PauseAudioStreamDevice(stream);
        }
    }

    void AudioTrack::Resume() {
        if (!m_handle) return;
        if (auto* stream = MIX_GetTrackAudioStream(m_handle)) {
            SDL_ResumeAudioStreamDevice(stream);
        }
    }

    void AudioTrack::Stop() {
        if (!m_handle) return;
        MIX_DestroyTrack(m_handle);
        m_handle = nullptr;
    }

    void AudioTrack::SetVolume(float gain) {
        if (!m_handle) return;
        m_gain = gain;
        MIX_SetTrackGain(m_handle, gain);
    }

    float AudioTrack::GetVolume() const {
        return m_gain;
    }

    void AudioTrack::SetLooping(int loopCount) {
        m_loops = loopCount;
        // Looping is set when creating the track, can't change mid-playback in SDL3_mixer
    }

    void AudioTrack::SetFadeIn(float seconds) {
        // SDL3_mixer: fade would be handled via SDL audio stream
        // TODO: implement fade when needed
    }

    bool AudioTrack::IsPlaying() const {
        return !IsFinished();
    }

    bool AudioTrack::IsPaused() const {
        return false; // TODO: track pause state
    }

    bool AudioTrack::IsFinished() const {
        if (!m_handle) return true;
        // tracks that haven't started or are done have 0 remaining
        Sint64 remaining = MIX_GetTrackRemaining(m_handle);
        return remaining <= 0;
    }
}
