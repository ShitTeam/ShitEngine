#pragma once
#include "../Core/Core.h"
#include "AudioTrack.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Shit {

class SHIT_API AudioTrackGroup {
public:
    ~AudioTrackGroup() = default;

    void PauseAll();
    void ResumeAll();
    void StopAll();

    void SetVolume(float gain);
    float GetVolume() const { return m_gain; }
    const std::string& GetName() const { return m_name; }

private:
    friend class AudioPlayer;
    AudioTrackGroup(const std::string& name) : m_name(name) {}
    void registerTrack(AudioTrack* track);
    void unregisterTrack(AudioTrack* track);

    std::string m_name;
    float m_gain = 1.0f;
    std::vector<AudioTrack*> m_tracks;
};

/**
 * @brief 音频播放器单例
 */
class SHIT_API AudioPlayer {
public:
    // ── 生命周期 ──
    static inline bool Init() { return GetInstance().init(); }
    static inline void Destroy() { GetInstance().destroy(); }
    static inline void Update() { GetInstance().update(); }

    // ── 轨道组 ──
    static inline AudioTrackGroup* CreateTrackGroup(const std::string& name) {
        return GetInstance().createTrackGroup(name);
    }
    static inline AudioTrackGroup* GetTrackGroup(const std::string& name) {
        return GetInstance().getTrackGroup(name);
    }

    // ── 播放 ──
    static inline AudioTrack* Play(const std::string& filePath, const std::string& group = "default") {
        auto& inst = GetInstance();
        return inst.play(filePath, inst.getTrackGroup(group));
    }

    // ── 全局控制 ──
    static inline void SetMasterVolume(float gain) { GetInstance().setMasterVolume(gain); }
    static inline float GetMasterVolume() { return GetInstance().m_masterGain; }
    static inline void PauseAll() { GetInstance().pauseAll(); }
    static inline void ResumeAll() { GetInstance().resumeAll(); }
    static inline void StopAll() { GetInstance().stopAll(); }

    // ── 单例 ──
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
    AudioTrack* play(const std::string& filePath, AudioTrackGroup* group);

    void setMasterVolume(float gain);
    void pauseAll();
    void resumeAll();
    void stopAll();

    struct MIX_Mixer* m_mixer = nullptr;
    bool m_isInited = false;
    float m_masterGain = 1.0f;

    std::unordered_map<std::string, std::unique_ptr<AudioTrackGroup>> m_groups;
    std::vector<std::unique_ptr<AudioTrack>> m_tracks;
    std::unordered_map<std::string, struct MIX_Audio*> m_audioCache;
};

} // namespace Shit
