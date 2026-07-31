#pragma once
#include "../Core/Core.h"
#include "../Core/Log.h"
#include "AudioTrack.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Shit {

class AudioTrackGroup {
public:
    ~AudioTrackGroup() = default;

    void pauseAll();
    void resumeAll();
    void stopAll();
    void setVolume(float gain);
    float getVolume() const { return m_gain; }
    const std::string& getName() const { return m_name; }

private:
    friend class AudioPlayer;
    friend class AudioTrack;
    AudioTrackGroup(const std::string& name) : m_name(name) {}
    void registerTrack(AudioTrack* track);
    void unregisterTrack(AudioTrack* track);

    std::string m_name;
    float m_gain = 1.0f;
    std::vector<AudioTrack*> m_tracks;
};

class SHIT_API AudioPlayer {
public:
    static inline bool Init() { return GetInstance().init(); }
    static inline void Destroy() { GetInstance().destroy(); }
    static inline void Update() { GetInstance().update(); }
    static inline AudioTrackGroup* CreateTrackGroup(const std::string& name) {
        return GetInstance().createTrackGroup(name);
    }
    static inline AudioTrackGroup* GetTrackGroup(const std::string& name) {
        return GetInstance().getTrackGroup(name);
    }
    static inline AudioTrack* Play(const std::string& filePath, const std::string& group = "default") {
        auto& inst = GetInstance();
        AudioTrackGroup* g = inst.getTrackGroup(group);
        if (!g) {
            ST_CORE_WARN("AudioPlayer::Play 指定的音频组 \"{}\" 不存在，将按无分组播放（不受该组音量/暂停控制）", group);
        }
        return inst.play(filePath, g);
    }
    static inline void SetMasterVolume(float gain) { GetInstance().setMasterVolume(gain); }
    static inline float GetMasterVolume() { return GetInstance().m_masterGain; }
    static inline void PauseAll() { GetInstance().pauseAll(); }
    static inline void ResumeAll() { GetInstance().resumeAll(); }
    static inline void StopAll() { GetInstance().stopAll(); }
    static inline void ApplyTrackGain(AudioTrack* track, const AudioTrackGroup* group) {
        GetInstance().applyTrackGain(track, group);
    }

    static AudioPlayer& GetInstance();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) = delete;
    AudioPlayer& operator=(AudioPlayer&&) = delete;

private:
    AudioPlayer() = default;
    ~AudioPlayer() = default;

    bool init();
    void destroy();
    void update();
    AudioTrackGroup* createTrackGroup(const std::string& name);
    AudioTrackGroup* getTrackGroup(const std::string& name);
    AudioTrack* play(const std::string& filePath, AudioTrackGroup* group, int loopCount = 0);
    void setMasterVolume(float gain);
    void pauseAll();
    void resumeAll();
    void stopAll();
    void applyTrackGain(AudioTrack* track, const AudioTrackGroup* group);

    struct MIX_Mixer* m_mixer = nullptr;
    bool m_isInited = false;
    float m_masterGain = 1.0f;
    std::unordered_map<std::string, std::unique_ptr<AudioTrackGroup>> m_groups;
    std::vector<std::unique_ptr<AudioTrack>> m_tracks;
};

} // namespace Shit
