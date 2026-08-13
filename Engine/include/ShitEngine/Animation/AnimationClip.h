#pragma once

#include "../Core/Core.h"
#include "../Render/SpriteSheet.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Shit
{
    /**
     * @brief 动画剪辑（可序列化数据，P28）
     *
     * 一份动画剪辑描述"从某张精灵表纹理按网格切出的帧序列 + 播放参数"。
     * 作为独立数据类，既可作为运行时/编辑器的动画数据，也可序列化为 `.anim` 资产
     * （AnimationClip 资产化）或内嵌进 Animator / AnimationComponent 的载体字符串。
     *
     * frames 为 SpriteSheet 全局帧索引序列（与 AnimationComponent::play 的 frames 语义一致）。
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
        float duration = 0.1f;            ///< 每帧时长（秒）
        bool loop = true;                 ///< 是否循环
        bool isDefault = false;           ///< 是否默认播放（onStart 自动播放）
        std::vector<int> frames;          ///< 全局帧索引序列

        /// 序列化到 JSON（载体 / .anim 资产）
        nlohmann::json toJson() const;
        /// 从 JSON 解析（失败返回 false，字段保持默认）
        bool fromJson(const nlohmann::json &j);
    };
}
