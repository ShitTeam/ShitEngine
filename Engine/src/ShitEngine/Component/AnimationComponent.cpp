#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/AnimationComponent.h"

#include "ShitEngine/Render/Animation.h"
#include "ShitEngine/Component/SpriteRenderer.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <utility>

namespace Shit {

    // AnimationClip 的 toJson/fromJson 实现在 Animation/AnimationClip.cpp

    AnimationComponent::AnimationComponent() = default;
    AnimationComponent::~AnimationComponent() = default;

    void AnimationComponent::onAttach() {
        Behavior::onAttach();  // 把自己注册进 BehaviorSystem
    }

    void AnimationComponent::onStart() {
        // 有可序列化剪辑：播放默认剪辑；否则若已有当前动画则把首帧应用上去
        if (m_clipsData.empty()) {
            if (m_currentAnimation) applyCurrentFrame();
            return;
        }
        // 载体非空但尚未解析（如运行时脚本直接构造）→ 解析一次
        if (m_clips.empty()) parseClipsData();
        if (m_clips.empty()) return;
        // 默认剪辑优先；无默认则播第一个
        int def = defaultClipIndex();
        if (def < 0) def = 0;
        if (def >= 0 && def < static_cast<int>(m_clips.size())) {
            const std::string& n = m_clips[static_cast<size_t>(def)].name;
            if (auto it = m_animations.find(n); it != m_animations.end() && it->second) {
                m_currentAnimation = it->second.get();
                m_currentAnimationName = n;
                m_currentTime = 0.0f;
                m_isPlaying = true;
                m_isPaused = false;
                applyCurrentFrame();
            }
        }
    }

    void AnimationComponent::onUpdate() {
        if (!m_isPlaying || m_isPaused || !m_currentAnimation) return;

        m_currentTime += Time::GetDeltaTime();

        // 非循环动画播到末尾：停在最后一帧并结束播放
        // 结束判断放在时间递增之后，保证播放时长严格等于 totalLen
        if (!m_currentAnimation->isLooping()) {
            const float totalLen = m_currentAnimation->getTotalDuration();
            if (totalLen > 0.0f && m_currentTime >= totalLen) {
                m_currentTime = totalLen;  // clamp 到末帧
                applyCurrentFrame();
                m_isPlaying = false;
                return;
            }
        }

        applyCurrentFrame();
    }

    void AnimationComponent::onDestroy() {
        Behavior::onDestroy();
        m_currentAnimation = nullptr;
        m_currentAnimationName.clear();
        m_animations.clear();
        m_clips.clear();
        m_isPlaying = false;
        m_isPaused = false;
    }

    void AnimationComponent::onAfterDeserialize() {
        // 反射直写 m_clipsData 后解析重建（与 Tilemap onAfterDeserialize 同模式）
        parseClipsData();
    }

    void AnimationComponent::onFieldChanged(const std::string& fieldName) {
        // 载体被直写（编辑器文本 / 脚本）→ 重新解析
        if (fieldName == "m_clipsData") {
            parseClipsData();
        }
    }

    void AnimationComponent::addAnimation(const std::string& animationName, std::unique_ptr<Animation> animation) {
        if (!animation) {
            ST_CORE_WARN("试图添加空动画指针!");
            return;
        }
        m_animations[animationName] = std::move(animation);
        // 覆盖当前正在播放的同名动画时，m_currentAnimation 需重新指向新对象，避免悬垂指针
        if (animationName == m_currentAnimationName) {
            m_currentAnimation = m_animations[animationName].get();
        }
    }

    void AnimationComponent::play(const std::string& name) {
        auto it = m_animations.find(name);
        if (it == m_animations.end() || !it->second) {
            ST_CORE_WARN("找不到名为 {} 的动画！", name);
            return;
        }
        // 动画名不同，或上次播放已自然结束（非循环播完 m_isPlaying==false），从头播放
        if (m_currentAnimationName != name || !m_isPlaying) {
            m_currentAnimation = it->second.get();
            m_currentAnimationName = name;
            m_currentTime = 0.0f;
        }
        m_isPlaying = true;
        m_isPaused = false;
        applyCurrentFrame();
    }

    void AnimationComponent::play(const std::string& name, const SpriteSheet& sheet,
                                  const std::vector<int>& frames, float duration, bool loop) {
        auto anim = std::make_unique<Animation>(duration, loop);
        for (int idx : frames) {
            anim->addFrame(sheet.getFrameRect(idx));
        }
        // 登记（存在则覆盖）并切换为当前
        m_animations[name] = std::move(anim);
        m_currentAnimation = m_animations[name].get();
        m_currentAnimationName = name;
        m_currentTime = 0.0f;
        m_isPlaying = true;
        m_isPaused = false;
        applyCurrentFrame();
    }

    void AnimationComponent::stop() {
        m_isPlaying = false;
        m_isPaused = false;
        m_currentTime = 0.0f;

        // 恢复 SpriteRenderer 为整图渲染
        if (auto* sprite = getOwner() ? getOwner()->getComponent<SpriteRenderer>() : nullptr) {
            sprite->setSourceRect(std::nullopt);
        }
    }

    void AnimationComponent::pause() {
        m_isPaused = true;
    }

    void AnimationComponent::resume() {
        if (!m_currentAnimation) return;
        m_isPaused = false;
    }

    void AnimationComponent::applyCurrentFrame() {
        if (!m_currentAnimation) return;
        if (m_currentAnimation->getFrameCount() == 0) return;

        // duration <= 0 时 getFrame 会安全返回首帧
        SDL_FRect frame = m_currentAnimation->getFrame(m_currentTime);

        auto* owner = getOwner();
        if (!owner) return;
        auto* sprite = owner->getComponent<SpriteRenderer>();
        if (sprite) sprite->setSourceRect(frame);
    }

    // ═══════════════════════════════════════════════════════════
    // P28 剪辑管理
    // ═══════════════════════════════════════════════════════════

    const AnimationClip* AnimationComponent::clipAt(int index) const {
        if (index < 0 || index >= static_cast<int>(m_clips.size())) return nullptr;
        return &m_clips[static_cast<size_t>(index)];
    }

    void AnimationComponent::parseClipsData() {
        m_clips.clear();
        if (m_clipsData.empty()) return;
        try {
            nlohmann::json arr = nlohmann::json::parse(m_clipsData);
            if (!arr.is_array()) return;
            for (const auto& item : arr) {
                AnimationClip clip;
                if (clip.fromJson(item))
                    m_clips.push_back(std::move(clip));
            }
        } catch (const std::exception& e) {
            ST_CORE_WARN("AnimationComponent: 解析 m_clipsData 失败: {}", e.what());
        }
        rebuildFromClips();
    }

    void AnimationComponent::rebuildFromClips() {
        // 以剪辑重建运行时动画（覆盖同名），不动 m_animations 里由 play() 动态添加的其它项
        for (const auto& clip : m_clips) {
            if (clip.frames.empty()) continue;
            SpriteSheet sheet(clip.rows, clip.cols, clip.frameWidth, clip.frameHeight,
                              clip.margin, clip.spacing);
            auto anim = std::make_unique<Animation>(clip.duration, clip.loop);
            for (int idx : clip.frames)
                anim->addFrame(sheet.getFrameRect(idx));
            // 逐帧独立时长（P29 Dope Sheet）：长度匹配时传给运行时
            if (clip.frameDurations.size() == clip.frames.size())
                anim->setFrameDurations(clip.frameDurations);
            m_animations[clip.name] = std::move(anim);
        }
    }

    void AnimationComponent::notifyClipsChanged() {
        rebuildFromClips();
        syncClipsData();
    }

    void AnimationComponent::syncClipsData() {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& clip : m_clips)
            arr.push_back(clip.toJson());
        m_clipsData = arr.dump();
    }

    int AnimationComponent::addClip(const std::string& name) {
        // 去重：重名追加 " (N)"
        std::string base = name.empty() ? "Clip" : name;
        std::string candidate = base;
        int n = 1;
        bool exists = true;
        while (exists) {
            exists = false;
            for (const auto& c : m_clips) {
                if (c.name == candidate) { exists = true; break; }
            }
            if (exists) candidate = base + " (" + std::to_string(n++) + ")";
        }
        // 空纹理/网格时给默认 1x1 帧占位，编辑器随后填充
        AnimationClip clip;
        clip.name = candidate;
        clip.rows = 1;
        clip.cols = 1;
        clip.frameWidth = 32.0f;
        clip.frameHeight = 32.0f;
        clip.frames = { 0 };
        m_clips.push_back(std::move(clip));
        int idx = static_cast<int>(m_clips.size()) - 1;
        // 首条剪辑自动设为默认
        if (idx == 0) m_clips[0].isDefault = true;
        notifyClipsChanged();
        return idx;
    }

    bool AnimationComponent::setClip(int index, const AnimationClip& clip) {
        if (index < 0 || index >= static_cast<int>(m_clips.size())) return false;
        m_clips[static_cast<size_t>(index)] = clip;
        notifyClipsChanged();
        return true;
    }

    bool AnimationComponent::removeClip(int index) {
        if (index < 0 || index >= static_cast<int>(m_clips.size())) return false;
        const std::string removedName = m_clips[static_cast<size_t>(index)].name;
        // 若正在播放该剪辑则停止
        if (m_currentAnimationName == removedName) {
            stop();
            m_currentAnimationName.clear();
        }
        m_clips.erase(m_clips.begin() + index);
        m_animations.erase(removedName);
        // 移除的是默认剪辑 → 重新指定第一条为默认
        if (m_clips.empty()) {
            // 无剪辑：清空载体
            m_clipsData.clear();
            return true;
        }
        bool hasDefault = false;
        for (const auto& c : m_clips) if (c.isDefault) { hasDefault = true; break; }
        if (!hasDefault) m_clips[0].isDefault = true;
        notifyClipsChanged();
        return true;
    }

    bool AnimationComponent::setDefaultClip(int index) {
        if (index < 0 || index >= static_cast<int>(m_clips.size())) return false;
        for (size_t i = 0; i < m_clips.size(); ++i)
            m_clips[i].isDefault = (static_cast<int>(i) == index);
        notifyClipsChanged();
        return true;
    }

    int AnimationComponent::defaultClipIndex() const {
        for (size_t i = 0; i < m_clips.size(); ++i)
            if (m_clips[i].isDefault) return static_cast<int>(i);
        return -1;
    }
}
