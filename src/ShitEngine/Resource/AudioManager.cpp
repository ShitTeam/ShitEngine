#include "ShitEngine/Resource/AudioManager.h"
#include "ShitEngine/Core/Log.h"

#include <SFML/Audio.hpp>

namespace Shit {
    sf::SoundBuffer* AudioManager::loadSound(const std::string& filePath) {
        // 检查是否已加载
		if (auto it = m_sounds.find(filePath); it != m_sounds.end()) {
			return it->second.get();
		}

        auto new_sound = std::make_unique<sf::SoundBuffer>();

        if (!new_sound->loadFromFile(filePath)) {
            ST_CORE_ERROR("无法加载 {}", filePath);
			return nullptr;
        }

        sf::SoundBuffer* ptr = new_sound.get();

		//插入纹理
		m_sounds.insert({ filePath, std::move(new_sound)});

		return ptr;
    }

    sf::SoundBuffer* AudioManager::getSound(const std::string& filePath) {
        // 检查是否已加载
		if (auto it = m_sounds.find(filePath); it != m_sounds.end()) {
			return it->second.get();
		}

		ST_CORE_ERROR("没有找到音频 {} ，正在加载 ...", filePath);

		return loadSound(filePath);
    }

    void AudioManager::unloadSound(const std::string& filePath) {
        if (auto it = m_sounds.find(filePath); it != m_sounds.end()) {
			ST_CORE_DEBUG("卸载音频 {}", filePath);
			m_sounds.erase(it);
		}
		else {
			ST_CORE_WARN("尝试卸载不存在的音频 {}", filePath);
		}
    }

    void AudioManager::clearSound() {
        if (!m_sounds.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的纹理。", m_sounds.size());
			m_sounds.clear();
		}
    }
}
