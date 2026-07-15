#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <SDL3_mixer/SDL_mixer.h>

#include "ShitEngine/Core/Core.h"

namespace Shit {
	class AudioManager final {
		friend class ResourceManager;
	public:
		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;
		AudioManager(AudioManager&&) = default;
		AudioManager& operator=(AudioManager&&) = default;
		~AudioManager() { clearAudio(); }

	private:
		AudioManager() = default;

		struct MIXAudioDeleter {
			void operator()(MIX_Audio* audio) const {
				if (audio) MIX_DestroyAudio(audio);
			}
		};

		MIX_Audio* loadAudio(const std::string& filePath);
		MIX_Audio* getAudio(const std::string& filePath);
		void unloadAudio(const std::string& filePath);
		void clearAudio();
		void setMixer(MIX_Mixer* mixer) { m_mixer = mixer; }

		MIX_Mixer* m_mixer = nullptr;
		std::unordered_map<std::string, std::unique_ptr<MIX_Audio, MIXAudioDeleter>> m_audioCache;
	};
}
