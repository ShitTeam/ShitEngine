#pragma once
#include "../Core/Core.h"
#include "../Component/Behavior.h"
#include <memory>
#include <string>

namespace Shit {
	class AudioTrack;

	/**
	 * @brief 音频播放组件 —— 挂在对象上播放场景音频
	 *
	 * 继承 Behavior（与 AnimationComponent 一致），由 BehaviorSystem 驱动：
	 * onStart 时按 PlayOnStart 自动播放；脚本中也可手动 play()/stop()/pause()。
	 *
	 * 音频路径与纹理同一约定（相对资源目录），播放句柄为共享引用——
	 * 播完引擎自动回收；重新 play() 新建轨道替换旧句柄。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) AudioSource : public Behavior {
		SHIT_REFLECT_BODY(AudioSource)
	public:
		AudioSource() = default;

		// --- 属性 ---
		void setAudioPath(const std::string& path);
		const std::string& getAudioPath() const { return m_audioPath; }

		void setLoop(bool loop);
		bool isLoop() const { return m_loop; }

		void setVolume(float volume);   ///< 音量 0~1
		float getVolume() const { return m_volume; }

		void setPlayOnStart(bool playOnStart);
		bool isPlayOnStart() const { return m_playOnStart; }

		// --- 播放控制 ---
		void play();   ///< 按 AudioPath 播放（遵循 Loop / Volume）
		void stop();
		void pause();

		// Behavior 生命周期：首次驱动时按 PlayOnStart 自动播放
		void onStart() override;

	private:
		SHIT_META(({.displayName = "Audio Path", .tooltip = "音频资源路径（相对资源目录，如 resource/bgm.mp3）"}))
		std::string m_audioPath;
		SHIT_META(({.displayName = "Loop", .tooltip = "循环播放"}))
		bool m_loop = false;
		SHIT_META(({.displayName = "Volume", .tooltip = "音量 0~1", .range = {0, 1}, .step = 0.05}))
		float m_volume = 1.0f;
		SHIT_META(({.displayName = "Play On Start", .tooltip = "onStart 时自动播放（全局暂停/编辑态不播）"}))
		bool m_playOnStart = true;

		// 当前播放轨道句柄（仅运行时，不入反射/序列化）
		SHIT_META(Disable)
		std::shared_ptr<AudioTrack> m_track;
	};
}