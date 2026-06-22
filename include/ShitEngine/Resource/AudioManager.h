// #pragma once

// #include <string>
// #include <unordered_map>
// #include <memory>
// #include <SDL3_mixer/SDL_mixer.h>


// namespace Shit {
//     /**
//      * @brief 音频管理器
//      */
//     class AudioManager final {
//         friend class ResourceManager;
//     public:
// 		AudioManager(const AudioManager&) = delete;
// 		AudioManager& operator=(const AudioManager&) = delete;
// 		AudioManager(AudioManager&&) = default;
// 		AudioManager& operator=(AudioManager&&) = default;
	
// 	private:
// 		AudioManager() = default;
// 		~AudioManager() = default;

// 		// --- Sound ---
// 		MIX_Audio* loadSound(const std::string& filePath);
// 		MIX_Audio* getSound(const std::string& filePath);
// 		void unloadSound(const std::string& filePath);
// 		void clearSound();

// 		// --- Music ---
// 		MIX_Audio* loadMusic(const std::string& filePath);
// 		MIX_Audio* getMusic(const std::string& filePath);
// 		void unloadMusic(const std::string& filePath);
// 		void clearMusic();

// 		struct MIXAudioDeleter {
// 			void operator()(MIX_Audio* audio) const {
// 				if (audio) {
// 					MIX_DestroyAudio(audio);
// 				}
// 			}
// 		};

//         // Sound缓存
//         std::unordered_map<std::string, std::unique_ptr<MIX_Audio, MIXAudioDeleter>> m_sounds;

// 		// Music缓存
// 		std::unordered_map<std::string, std::unique_ptr<MIX_Audio, MIXAudioDeleter>> m_musics;
//     };
// }