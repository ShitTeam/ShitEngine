#include "ShitEngine/Resource/AudioManager.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/pch.h"

namespace Shit {
	MIX_Audio* AudioManager::loadAudio(const std::string& filePath) {
		if (auto it = m_audioCache.find(filePath); it != m_audioCache.end()) {
			return it->second.get();
		}

		if (!m_mixer) {
			ST_CORE_ERROR("AudioManager 未设置混音器，无法加载音频: {}", filePath);
			return nullptr;
		}

		auto audio = std::unique_ptr<MIX_Audio, MIXAudioDeleter>(MIX_LoadAudio(m_mixer, filePath.c_str(), true));
		if (!audio) {
			ST_CORE_ERROR("无法加载音频 {}", filePath);
			return nullptr;
		}

		MIX_Audio* ptr = audio.get();
		m_audioCache[filePath] = std::move(audio);
		return ptr;
	}

	MIX_Audio* AudioManager::getAudio(const std::string& filePath) {
		if (auto it = m_audioCache.find(filePath); it != m_audioCache.end()) {
			return it->second.get();
		}

		ST_CORE_ERROR("没有找到音频 {} ，正在加载 ...", filePath);
		return loadAudio(filePath);
	}

	void AudioManager::unloadAudio(const std::string& filePath) {
		if (auto it = m_audioCache.find(filePath); it != m_audioCache.end()) {
			ST_CORE_DEBUG("卸载音频 {}", filePath);
			m_audioCache.erase(it);
		} else {
			ST_CORE_WARN("尝试卸载不存在的音频 {}", filePath);
		}
	}

	void AudioManager::clearAudio() {
		if (!m_audioCache.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的音频。", m_audioCache.size());
			m_audioCache.clear();
		}
	}
}
