#pragma once

#include "../Core/Core.h"
#include "../Render/SpriteSheet.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Shit
{
    /// 动画帧（参考 Unity：每帧引用一个 Sprite = 纹理路径 + 源矩形）
    /// texturePath 空 = 沿用 SpriteRenderer 当前纹理（兼容动态 play(name, sheet, frames) API）
    struct SHIT_API AnimFrame {
        std::string texturePath;
        Rect rect;
    };

    /**
     * @brief 动画剪辑（可序列化数据，P28）
     *
     * 一份动画剪辑描述"从某张精灵表纹理按网格切出的帧序列 + 播放参数"。
     * 作为独立数据类，既可作为运行时/编辑器的动画数据，也可序列化为 `.anim` 资产
     * （AnimationClip 资产化）或内嵌进 Animator / AnimationComponent 的载体字符串。
     *
     * frames 为 SpriteSheet 全局帧索引序列（与 AnimationComponent::play 的 frames 语义一致）。
     * frameSprites 为运行时真值（每帧自带纹理路径，支持跨图集多纹理）；
     * 旧格式（frames + 网格 + texturePath）加载时由 fromJson 自动展开为 frameSprites。
     */
    struct SHIT_API AnimationClip {
        std::string name;                 ///< 剪辑名（play/索引用）
        std::string texturePath;          ///< 精灵表纹理路径（编辑器预览与运行时切帧用）
        int rows = 0;                     ///< 网格行数
        int cols = 0;                     ///< 网格列数
        float frameWidth = 0.0f;          ///< 单帧宽（像素）
        float frameHeight = 0.0f;         ///< 单帧高（像素）
        float margin = 0.0f;              ///< 网格左上留白（像素）
        float spacing = 0.0f;             ///< 帧间距（像素）
        float duration = 0.1f;            ///< 统一每帧时长（秒，无 frameDurations 时使用）
        bool loop = true;                 ///< 是否循环
        bool isDefault = false;           ///< 是否默认播放（onStart 自动播放）
        std::vector<int> frames;          ///< 全局帧索引序列（旧格式 / 单图集快捷选帧）
        std::vector<float> frameDurations;///< 每帧独立时长（秒，可选）。长度与 frameSprites 等长时逐帧生效，
                                          ///< 否则回退到统一 duration（Dope Sheet 时间轴，P29）
        std::vector<AnimFrame> frameSprites;  ///< 运行时真值：每帧 {纹理路径 + 源矩形}，支持跨图集多纹理。
                                              ///< 空 = 旧格式，由 fromJson 从 frames+网格+texturePath 展开

        /// 序列化到 JSON（载体 / .anim 资产）
        nlohmann::json toJson() const;
        /// 从 JSON 解析（失败返回 false，字段保持默认）
        bool fromJson(const nlohmann::json &j);
        /// 若 frameSprites 为空且 frames 非空，用网格+texturePath 展开填充 frameSprites（幂等）
        void expandFramesToSprites();
    };
}
