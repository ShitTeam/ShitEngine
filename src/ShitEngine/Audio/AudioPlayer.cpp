#include "ShitEngine/Audio/AudioPlayer.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_audio.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace Shit {

// ═══════════════════════════════════════════
// AudioTrackGroup
// ═══════════════════════════════════════════

void AudioTrackGroup::registerTrack(AudioTrack* track) {
    if (track) m_tracks.push_back(track);
}

void AudioTrackGroup::unregisterTrack(AudioTrack* track) {
    m_tracks.erase(
        std::remove(m_tracks.begin(), m_tracks.end(), track),
        m_tracks.end()
    );
}

void AudioTrackGroup::PauseAll() {
    for (auto* track : m_tracks) track->Pause();
}

void AudioTrackGroup::ResumeAll() {
    for (auto* track : m_tracks) track->Resume();
}

void AudioTrackGroup::StopAll() {
    for (auto* track : m_tracks) track->Stop();
    m_tracks.clear();
}

void AudioTrackGroup::SetVolume(float gain) {
    m_gain = gain;
    for (auto* track : m_tracks) {
        track->SetVolume(gain);
    }
}

// ═══════════════════════════════════════════
// AudioPlayer
// ═══════════════════════════════════════════

AudioPlayer& AudioPlayer::GetInstance() {
    static AudioPlayer instance;
    return instance;
}

bool AudioPlayer::init() {
    if (m_isInited) return true;

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        ST_CORE_ERROR("AudioPlayer 创建混音器失败: {}", SDL_GetError());
        return false;
    }

    m_groups["default"] = std::unique_ptr<AudioTrackGroup>(new AudioTrackGroup("default"));

    m_isInited = true;
    ST_CORE_TRACE("AudioPlayer 初始化成功");
    return true;
}

void AudioPlayer::destroy() {
    stopAll();
    m_tracks.clear();
    m_groups.clear();

    for (auto& [path, audio] : m_audioCache) {
        if (audio) MIX_DestroyAudio(audio);
    }
    m_audioCache.clear();

    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
    }
    m_isInited = false;
    ST_CORE_TRACE("AudioPlayer 已销毁");
}

void AudioPlayer::update() {
    m_tracks.erase(
        std::remove_if(m_tracks.begin(), m_tracks.end(),
            [](const std::unique_ptr<AudioTrack>& track) {
                if (track && track->IsFinished()) {
                    track->Stop();
                    return true;
                }
                return false;
            }),
        m_tracks.end()
    );
}

AudioTrackGroup* AudioPlayer::createTrackGroup(const std::string& name) {
    auto it = m_groups.find(name);
    if (it != m_groups.end()) return it->second.get();

    auto group = std::unique_ptr<AudioTrackGroup>(new AudioTrackGroup(name));
    auto* ptr = group.get();
    m_groups[name] = std::move(group);
    return ptr;
}

AudioTrackGroup* AudioPlayer::getTrackGroup(const std::string& name) {
    auto it = m_groups.find(name);
    return it != m_groups.end() ? it->second.get() : nullptr;
}

AudioTrack* AudioPlayer::play(const std::string& filePath, AudioTrackGroup* group) {
    if (!m_isInited || !m_mixer) {
        ST_CORE_ERROR("AudioPlayer 未初始化");
        return nullptr;
    }

    MIX_Audio* audio = nullptr;
    auto cacheIt = m_audioCache.find(filePath);
    if (cacheIt != m_audioCache.end()) {
        audio = cacheIt->second;
    } else {
        audio = MIX_LoadAudio(m_mixer, filePath.c_str(), true);
        if (!audio) {
            ST_CORE_ERROR("AudioPlayer 加载音频失败: {} ({})", filePath, SDL_GetError());
            return nullptr;
        }
        m_audioCache[filePath] = audio;
    }

    MIX_Track* handle = MIX_CreateTrack(m_mixer);
    if (!handle) {
        ST_CORE_ERROR("AudioPlayer 创建 Track 失败: {}", SDL_GetError());
        return nullptr;
    }

    MIX_SetTrackAudio(handle, audio);
    MIX_PlayTrack(handle, 0);

    auto track = std::unique_ptr<AudioTrack>(new AudioTrack(handle));
    track->SetVolume(m_masterGain * (group ? group->GetVolume() : 1.0f));
    auto* trackPtr = track.get();

    m_tracks.push_back(std::move(track));
    if (group) group->registerTrack(trackPtr);

    return trackPtr;
}

void AudioPlayer::setMasterVolume(float gain) {
    m_masterGain = gain;
    for (auto& track : m_tracks) {
        track->SetVolume(m_masterGain);
    }
}

void AudioPlayer::pauseAll() {
    for (auto& track : m_tracks) track->Pause();
}

void AudioPlayer::resumeAll() {
    for (auto& track : m_tracks) track->Resume();
}

void AudioPlayer::stopAll() {
    for (auto& group : m_groups) group.second->m_tracks.clear();
    for (auto& track : m_tracks) track->Stop();
    m_tracks.clear();
}

} // namespace Shit
