#pragma once

#include <ShitEngine/Audio/AudioSource.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_AudioSource() {
    Shit::ReflectType("AudioSource", sizeof(AudioSource))
        .Base("Behavior")
        .Field("m_audioPath",
            &Shit::AudioSource::m_audioPath, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Audio Path", .tooltip = "音频资源路径（相对资源目录，如 resource/bgm.mp3）"})
        .Field("m_loop",
            &Shit::AudioSource::m_loop, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Loop", .tooltip = "循环播放"})
        .Field("m_volume",
            &Shit::AudioSource::m_volume, "float")
        .Meta(Shit::FieldMeta{.displayName = "Volume", .tooltip = "音量 0~1", .range = {0, 1}, .step = 0.05})
        .Field("m_playOnStart",
            &Shit::AudioSource::m_playOnStart, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Play On Start", .tooltip = "onStart 时自动播放（全局暂停/编辑态不播）"})
        .Factory<AudioSource>()
        .Register<AudioSource>();
    return true;
}

} // namespace Shit
