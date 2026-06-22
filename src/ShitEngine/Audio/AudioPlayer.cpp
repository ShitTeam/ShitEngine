// #include "ShitEngine/Audio/AudioPlayer.h"

// #include "ShitEngine/Core/pch.h"
// #include "ShitEngine/Core/Log.h"

// namespace Shit
// {
//     AudioPlayer::AudioPlayer() = default;

//     AudioPlayer& AudioPlayer::GetInstance() {
//         static AudioPlayer instance;
//         return instance;
//     }

//     bool AudioPlayer::init(){
//         if (!MIX_Init()) {
//             ST_CORE_ERROR("AudioPlayer 错误：MIX_Init 初始化失败：{}", std::string(SDL_GetError()));
//             return false;
//         }

//         // 创建混音器
//         m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
//         if (!m_mixer) {
//             MIX_Quit();
//             ST_CORE_ERROR("AudioPlayer 错误：混音器初始化失败：{}", std::string(SDL_GetError()));
//             return false;
//         }

//         ST_CORE_TRACE("AudioPlayer 初始化成功。");
//         return true;
//     }
// }
