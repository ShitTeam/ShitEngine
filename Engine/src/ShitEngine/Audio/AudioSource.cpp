#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Audio/AudioSource.h"

#include "ShitEngine/Audio/AudioPlayer.h"
#include "ShitEngine/Audio/AudioTrack.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {

	void AudioSource::setAudioPath(const std::string& path) {
		m_audioPath = path;
	}

	void AudioSource::setLoop(bool loop) {
		m_loop = loop;
	}

	void AudioSource::setVolume(float volume) {
		m_volume = std::clamp(volume, 0.0f, 1.0f);
	}

	void AudioSource::setPlayOnStart(bool playOnStart) {
		m_playOnStart = playOnStart;
	}

	void AudioSource::play() {
		if (m_audioPath.empty()) {
			ST_CORE_WARN("[AudioSource] audioPath 为空，无法播放");
			return;
		}
		m_track = Shit::AudioPlayer::Play(m_audioPath, "default");
		if (m_track) {
			m_track->setLooping(m_loop ? -1 : 0);
			m_track->setVolume(m_volume);
		}
	}

	void AudioSource::stop() {
		if (m_track) m_track->stop();
	}

	void AudioSource::pause() {
		if (m_track) m_track->pause();
	}

	void AudioSource::onStart() {
		Behavior::onStart();
		// 编辑态（全局暂停）自动播放会每帧打扰：仅非暂停（运行/播放态）才出声
		if (m_playOnStart && !Shit::Game::IsPaused()) {
			play();
		}
	}
}