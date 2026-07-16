#include "ShitEngine/Audio/AudioPlayer.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Resource/ResourceManager.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_properties.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace Shit {

// ═══════════════════════════════════════════
// AudioTrackGroup
// ═══════════════════════════════════════════

void AudioTrackGroup::registerTrack(AudioTrack* track) {
    if (track) {
        m_tracks.push_back(track);
        track->m_group = this;  // 友元写入
    }
}

void AudioTrackGroup::unregisterTrack(AudioTrack* track) {
    if (!track) return;
    m_tracks.erase(
        std::remove(m_tracks.begin(), m_tracks.end(), track),
        m_tracks.end()
    );
    if (track) track->m_group = nullptr;
}

void AudioTrackGroup::pauseAll() {
    for (auto* track : m_tracks) track->pause();
}

void AudioTrackGroup::resumeAll() {
    for (auto* track : m_tracks) track->resume();
}

void AudioTrackGroup::stopAll() {
    for (auto* track : m_tracks) {
        track->m_group = nullptr;  // 避免后续 unregister 回写本已清空的列表
        track->stop();
    }
    m_tracks.clear();
}

void AudioTrackGroup::setVolume(float gain) {
    m_gain = gain;
    for (auto* track : m_tracks) {
        AudioPlayer::ApplyTrackGain(track, this);
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

    if (!MIX_Init()) {
        ST_CORE_ERROR("AudioPlayer MIX_Init 失败: {}", SDL_GetError());
        return false;
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        ST_CORE_ERROR("AudioPlayer 创建混音器失败: {}", SDL_GetError());
        MIX_Quit();
        return false;
    }

    ResourceManager::SetAudioMixer(m_mixer);
    m_groups["default"] = std::unique_ptr<AudioTrackGroup>(new AudioTrackGroup("default"));

    m_isInited = true;
    ST_CORE_TRACE("AudioPlayer 初始化成功");
    return true;
}

void AudioPlayer::destroy() {
    stopAll();
    m_tracks.clear();
    m_groups.clear();

    // 在 MIX_Quit 前清空音频缓存，避免 MIX_DestroyAudio 在 MIX_Quit 后调用
    ResourceManager::ClearAudio();
    ResourceManager::SetAudioMixer(nullptr);

    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
    }
    MIX_Quit();
    m_isInited = false;
    ST_CORE_TRACE("AudioPlayer 已销毁");
}

void AudioPlayer::update() {
    // 先识别已完成 track，从其所属 group 注销（避免悬空指针），再停止并移除
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        AudioTrack* track = it->get();
        if (track && track->isFinished()) {
            if (track->m_group) {
                track->m_group->unregisterTrack(track);  // 会清掉 track->m_group
            }
            track->stop();
            it = m_tracks.erase(it);
        } else {
            ++it;
        }
    }
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

AudioTrack* AudioPlayer::play(const std::string& filePath, AudioTrackGroup* group, int loopCount) {
    if (!m_isInited || !m_mixer) {
        ST_CORE_ERROR("AudioPlayer 未初始化");
        return nullptr;
    }

    MIX_Audio* audio = ResourceManager::GetAudio(filePath);
    if (!audio) {
        ST_CORE_ERROR("AudioPlayer 加载音频失败: {}", filePath);
        return nullptr;
    }

    MIX_Track* handle = MIX_CreateTrack(m_mixer);
    if (!handle) {
        ST_CORE_ERROR("AudioPlayer 创建 Track 失败: {}", SDL_GetError());
        return nullptr;
    }

    MIX_SetTrackAudio(handle, audio);

    // 启动播放：当要求循环（loopCount != 0）时通过 properties 写入
    // MIX_PROP_PLAY_LOOPS_NUMBER，使循环随 PlayTrack 生效（而非仅在播放中改）。
    // loopCount: 0=不循环(默认), -1=无限, N=循环 N 次。
    if (loopCount != 0) {
        SDL_PropertiesID props = SDL_CreateProperties();
        if (props) {
            SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, static_cast<Sint64>(loopCount));
        }
        MIX_PlayTrack(handle, props ? props : 0);
        if (props) SDL_DestroyProperties(props);
    } else {
        MIX_PlayTrack(handle, 0);
    }

    auto track = std::unique_ptr<AudioTrack>(new AudioTrack(handle));
    auto* trackPtr = track.get();
    trackPtr->m_loops = loopCount;  // 记录循环数，供 setLooping/get 使用

    m_tracks.push_back(std::move(track));
    if (group) group->registerTrack(trackPtr);  // 同时置 trackPtr->m_group = group

    // 下发初始层级增益 master × group × track(1.0)
    applyTrackGain(trackPtr, group);

    return trackPtr;
}

void AudioPlayer::setMasterVolume(float gain) {
    m_masterGain = gain;
    // 每条 track 重新计算 master × group × track
    for (auto& track : m_tracks) {
        applyTrackGain(track.get(), track->m_group);
    }
}

void AudioPlayer::pauseAll() {
    for (auto& track : m_tracks) track->pause();
}

void AudioPlayer::resumeAll() {
    for (auto& track : m_tracks) track->resume();
}

void AudioPlayer::stopAll() {
    // 先断开 group 对 track 的引用，再停止；避免 group 列表留下悬空指针
    for (auto& group : m_groups) group.second->m_tracks.clear();
    for (auto& track : m_tracks) {
        track->m_group = nullptr;
        track->stop();
    }
    m_tracks.clear();
}

void AudioPlayer::applyTrackGain(AudioTrack* track, const AudioTrackGroup* group) {
    if (!track || !track->m_handle) return;
    const float groupGain = group ? group->getVolume() : 1.0f;
    const float gain = m_masterGain * groupGain * track->getVolume();
    MIX_SetTrackGain(track->m_handle, gain);
}

} // namespace Shit
