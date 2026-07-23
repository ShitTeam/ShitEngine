#pragma once

#include "Behavior.h"
#include "../Core/Core.h"
#include "../Render/SpriteSheet.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Shit
{
    class Animation;

    /**
     * @brief 动画组件
     *
     * 继承 Behavior，由 BehaviorSystem 每帧调用 onUpdate 推进当前动画时间，
     * 并把当前帧的源矩形 (SDL_FRect) 回写到同 GameObject 上的 SpriteRenderer。
     *
     * 典型用法（三点式 sprite-sheet）：
     *   Shit::SpriteSheet sheet(4, 8, 32, 32);            // 4行8列，每帧32×32
     *   auto* anim = go->addComponent<AnimationComponent>();
     *   anim->play("walk", sheet, {0,1,2,3,4,5}, 0.1f, true);  // 全局帧索引数组
     *
     * 或手工添加已构造好的 Animation：
     *   anim->addAnimation("walk", std::move(walkAnim));
     *   anim->play("walk");
     */
    class SHIT_API AnimationComponent : public Behavior {
    public:
        AnimationComponent();
        ~AnimationComponent() override;

        // --- 生命周期 ---
        void onAttach() override;
        void onStart() override;
        void onUpdate() override;
        void onDestroy() override;

        // --- 动画管理 ---
        void addAnimation(const std::string& name, std::unique_ptr<Animation> animation);

        void play(const std::string& name);
        void stop();
        void pause();
        void resume();

        /**
         * @brief 用数字帧索引直接设置并播放一段动画
         *
         * 按 frames 中的索引从 sheet 切出条帧，构造一个 Animation 并以 name 登记/覆盖后播放。
         * 若 name 已存在则替换。
         *
         * @param name 动画名（用作登记与 play 索引）
         * @param sheet 精灵图集，提供行列网格切割
         * @param frames 想要播放的全局帧索引序列（如 {0,1,2,3,5,8}，可不连续）
         * @param duration 每帧持续时间（秒）
         * @param loop 是否循环
         */
        void play(const std::string& name, const SpriteSheet& sheet,
                  const std::vector<int>& frames, float duration = 0.1f, bool loop = true);

        // --- 查询 ---
        bool isPlaying() const { return m_isPlaying; }
        bool isPaused() const { return m_isPaused; }
        const std::string& getCurrentAnimationName() const { return m_currentAnimationName; }

    private:
        // 把当前帧源矩形回写到 SpriteRenderer（若存在且纹理一致）
        void applyCurrentFrame();

        std::unordered_map<std::string, std::unique_ptr<Animation>> m_animations;
        Animation* m_currentAnimation = nullptr;
        std::string m_currentAnimationName;
        float m_currentTime = 0.0f;

        bool m_isPlaying = false;
        bool m_isPaused = false;
    };
}
