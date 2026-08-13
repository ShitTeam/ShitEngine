#pragma once

#include "Behavior.h"
#include "../Core/Core.h"
#include "../Animation/AnimationClip.h"
#include "../Render/SpriteSheet.h"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Shit
{
    class Animation;

    /**
     * @brief 动画组件 / Animator（可序列化多剪辑版本）
     *
     * 继承 Behavior，由 BehaviorSystem 每帧调用 onUpdate 推进当前动画时间，
     * 并把当前帧的源矩形 (SDL_FRect) 回写到同 GameObject 上的 SpriteRenderer。
     *
     * P28：可序列化的动画剪辑管理。动画剪辑（AnimationClip，独立于 Animation/AnimationClip.h）
     * 以反射字符串载体 m_clipsData（JSON）持久化，onAfterDeserialize / onFieldChanged 把其解析
     * 到内部 Animation 列表；onStart 自动播放默认剪辑。编辑器通过 addClip / setClip / removeClip /
     * setDefaultClip 等 API 修改并反向同步 m_clipsData。
     *
     * 运行时也可用原有 API 动态播放：
     *   anim->play("walk", sheet, {0,1,2,3,4,5}, 0.1f, true);  // 全局帧索引数组
     *
     * 注：如需参数驱动的状态机（状态/转换/参数），请用 Animation/Animator.h。
     */
    class SHIT_API SHIT_REFLECT(BlackList) AnimationComponent : public Behavior {
        SHIT_REFLECT_BODY(AnimationComponent)
    public:
        AnimationComponent();
        ~AnimationComponent() override;

        // --- 生命周期 ---
        void onAttach() override;
        void onStart() override;
        void onUpdate() override;
        void onDestroy() override;
        void onAfterDeserialize() override;
        void onFieldChanged(const std::string& fieldName) override;

        // --- 动画管理（运行时动态 API，保持兼容） ---
        void addAnimation(const std::string& name, std::unique_ptr<Animation> animation);

        void play(const std::string& name);
        void stop();
        void pause();
        void resume();

        /**
         * @brief 用数字帧索引直接设置并播放一段动画（运行时动态，不落盘）
         */
        void play(const std::string& name, const SpriteSheet& sheet,
                  const std::vector<int>& frames, float duration = 0.1f, bool loop = true);

        // --- P28 剪辑管理（编辑器 + 序列化） ---

        /// 剪辑数量
        int clipCount() const { return static_cast<int>(m_clips.size()); }
        /// 取第 i 个剪辑（越界返回 nullptr）
        const AnimationClip* clipAt(int index) const;
        /// 全部剪辑（只读）
        const std::vector<AnimationClip>& clips() const { return m_clips; }

        /// 新增一个剪辑（重名自动追加 " (N)"），返回其索引；-1 失败
        int addClip(const std::string& name);
        /// 覆盖第 index 个剪辑为 clip（并重建运行时动画）；返回是否成功
        bool setClip(int index, const AnimationClip& clip);
        /// 移除第 index 个剪辑（若正播放则停止）；返回是否成功
        bool removeClip(int index);
        /// 设为/取消默认剪辑（isDefault 唯一化）；返回是否成功
        bool setDefaultClip(int index);
        /// 取默认剪辑索引；无则 -1
        int defaultClipIndex() const;

        /// 把当前 m_clips 同步到 m_clipsData（JSON 载体）——编辑器/脚本修改后调用
        void syncClipsData();

        // --- 查询 ---
        bool isPlaying() const { return m_isPlaying; }
        bool isPaused() const { return m_isPaused; }
        const std::string& getCurrentAnimationName() const { return m_currentAnimationName; }

    private:
        // 把当前帧源矩形回写到 SpriteRenderer（若存在且纹理一致）
        void applyCurrentFrame();
        // 解析 m_clipsData 到 m_clips 并重建运行时 Animation 列表
        void parseClipsData();
        // 把 m_clips 重建为 m_animations（覆盖同名）；供 onStart/onAfterDeserialize/剪辑变更后调用
        void rebuildFromClips();
        // 剪辑数据变化后的统一入口：重建 + 同步载体
        void notifyClipsChanged();

        SHIT_META(Disable)
        std::unordered_map<std::string, std::unique_ptr<Animation>> m_animations;
        SHIT_META(Disable)
        Animation* m_currentAnimation = nullptr;
        // P28：运行时剪辑列表（从 m_clipsData 解析而来）
        SHIT_META(Disable)
        std::vector<AnimationClip> m_clips;
        // P28：序列化载体——所有剪辑的 JSON 数组字符串
        SHIT_META(({.displayName = "Animation Clips", .tooltip = "序列化载体，由编辑器维护（JSON）", .readOnly = true}))
        std::string m_clipsData;

        SHIT_META(({.displayName = "Current Animation", .readOnly = true}))
        std::string m_currentAnimationName;
        SHIT_META(({.displayName = "Current Time", .readOnly = true}))
        float m_currentTime = 0.0f;

        SHIT_META(({.displayName = "Playing", .readOnly = true}))
        bool m_isPlaying = false;
        SHIT_META(({.displayName = "Paused", .readOnly = true}))
        bool m_isPaused = false;
    };
}
