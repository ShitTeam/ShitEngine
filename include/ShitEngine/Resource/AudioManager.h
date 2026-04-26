#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace sf {
    class SoundBuffer;
}

namespace Shit {
    /**
     * @brief 音频管理器
     */
    class AudioManager final {
        friend class ResourceManager;
    public:
		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;
		AudioManager(AudioManager&&) = default;
		AudioManager& operator=(AudioManager&&) = default;
	
	private:
		AudioManager() = default;
		~AudioManager() = default;

		sf::SoundBuffer* loadSound(const std::string& filePath);
		sf::SoundBuffer* getSound(const std::string& filePath);
		void unloadSound(const std::string& filePath);
		void clearSound();

        // Sound缓存
        std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> m_sounds;
    };
}