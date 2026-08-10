#pragma once
#include "../Core/Core.h"
#include "../Core/Log.h"
#include "AudioTrack.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Shit {

// enable_shared_from_this：track 持 group 的 shared_ptr（m_groupOwner）拉长组生命周期，
// 避免 AudioPlayer::destroy 先释放 group 后，仍被外部持有的 track 析构时访问已释放的组。
class AudioTrackGroup : public std::enable_shared_from_this<AudioTrackGroup> {
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
    /// @brief 播放音频，返回共享句柄。播放结束后引擎自动回收资源；
    /// 调用方保存返回值可安全持有（track 播完后 isFinished()==true，
    /// 但对象不会悬垂——引擎释放的是自己的引用，调用方持有期间对象存活）。
    static inline std::shared_ptr<AudioTrack> Play(const std::string& filePath, const std::string& group = "default") {
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
    friend class EngineContext;
    AudioPlayer() = default;
    ~AudioPlayer() = default;

    bool init();
    void destroy();
    void update();
    AudioTrackGroup* createTrackGroup(const std::string& name);
    AudioTrackGroup* getTrackGroup(const std::string& name);
    std::shared_ptr<AudioTrack> play(const std::string& filePath, AudioTrackGroup* group, int loopCount = 0);
    void setMasterVolume(float gain);
    void pauseAll();
    void resumeAll();
    void stopAll();
    void applyTrackGain(AudioTrack* track, const AudioTrackGroup* group);

    struct MIX_Mixer* m_mixer = nullptr;
    bool m_isInited = false;
    float m_masterGain = 1.0f;
    std::unordered_map<std::string, std::shared_ptr<AudioTrackGroup>> m_groups;
    std::vector<std::shared_ptr<AudioTrack>> m_tracks;
};

} // namespace Shit
