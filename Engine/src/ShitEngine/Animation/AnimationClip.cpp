#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Animation/AnimationClip.h"
#include "ShitEngine/Render/SpriteSheet.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Shit {

    void AnimationClip::expandFramesToSprites() {
        if (!frameSprites.empty() || frames.empty()) return;
        SpriteSheet sheet(rows, cols, frameWidth, frameHeight, margin, spacing);
        for (int idx : frames)
            frameSprites.push_back(AnimFrame{ texturePath, sheet.getFrameRect(idx) });
    }

    nlohmann::json AnimationClip::toJson() const {
        nlohmann::json j;
        j["name"] = name;
        j["texturePath"] = texturePath;
        j["rows"] = rows;
        j["cols"] = cols;
        j["frameWidth"] = frameWidth;
        j["frameHeight"] = frameHeight;
        j["margin"] = margin;
        j["spacing"] = spacing;
        j["duration"] = duration;
        j["loop"] = loop;
        j["isDefault"] = isDefault;
        // 帧数据：新格式（frameSprites，每帧 path+rect）优先；空则写旧 frames 索引
        if (!frameSprites.empty()) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& f : frameSprites) {
                nlohmann::json fsObj;
                fsObj["texturePath"] = f.texturePath;
                nlohmann::json rectObj;
                rectObj["x"] = f.rect.x;
                rectObj["y"] = f.rect.y;
                rectObj["w"] = f.rect.w;
                rectObj["h"] = f.rect.h;
                fsObj["rect"] = rectObj;
                arr.push_back(std::move(fsObj));
            }
            j["frameSprites"] = std::move(arr);
        } else {
            j["frames"] = frames;
        }
        if (!frameDurations.empty()) j["frameDurations"] = frameDurations;
        return j;
    }

    bool AnimationClip::fromJson(const nlohmann::json& j) {
        if (!j.is_object()) return false;
        try {
            if (j.contains("name") && j["name"].is_string()) name = j["name"].get<std::string>();
            if (j.contains("texturePath") && j["texturePath"].is_string()) texturePath = j["texturePath"].get<std::string>();
            if (j.contains("rows") && j["rows"].is_number_integer()) rows = j["rows"].get<int>();
            if (j.contains("cols") && j["cols"].is_number_integer()) cols = j["cols"].get<int>();
            if (j.contains("frameWidth") && j["frameWidth"].is_number()) frameWidth = j["frameWidth"].get<float>();
            if (j.contains("frameHeight") && j["frameHeight"].is_number()) frameHeight = j["frameHeight"].get<float>();
            if (j.contains("margin") && j["margin"].is_number()) margin = j["margin"].get<float>();
            if (j.contains("spacing") && j["spacing"].is_number()) spacing = j["spacing"].get<float>();
            if (j.contains("duration") && j["duration"].is_number()) duration = j["duration"].get<float>();
            if (j.contains("loop") && j["loop"].is_boolean()) loop = j["loop"].get<bool>();
            if (j.contains("isDefault") && j["isDefault"].is_boolean()) isDefault = j["isDefault"].get<bool>();

            // 帧数据：新格式（frameSprites）优先；否则旧格式（frames + 网格）展开为 frameSprites
            frames.clear();
            frameSprites.clear();
            if (j.contains("frameSprites") && j["frameSprites"].is_array()) {
                for (const auto& fs : j["frameSprites"]) {
                    if (!fs.is_object()) continue;
                    AnimFrame f;
                    if (fs.contains("texturePath") && fs["texturePath"].is_string())
                        f.texturePath = fs["texturePath"].get<std::string>();
                    if (fs.contains("rect") && fs["rect"].is_object()) {
                        const auto& r = fs["rect"];
                        if (r.contains("x") && r["x"].is_number()) f.rect.x = r["x"].get<float>();
                        if (r.contains("y") && r["y"].is_number()) f.rect.y = r["y"].get<float>();
                        if (r.contains("w") && r["w"].is_number()) f.rect.w = r["w"].get<float>();
                        if (r.contains("h") && r["h"].is_number()) f.rect.h = r["h"].get<float>();
                    }
                    frameSprites.push_back(std::move(f));
                }
            } else if (j.contains("frames") && j["frames"].is_array()) {
                for (const auto& f : j["frames"])
                    if (f.is_number_integer()) frames.push_back(f.get<int>());
                expandFramesToSprites();   // 旧格式展开为 frameSprites（运行时真值）
            }

            if (j.contains("frameDurations") && j["frameDurations"].is_array()) {
                frameDurations.clear();
                for (const auto& d : j["frameDurations"])
                    if (d.is_number()) frameDurations.push_back(d.get<float>());
                // 长度与帧数不匹配则丢弃（避免脏数据影响播放）
                if (frameDurations.size() != frameSprites.size()) frameDurations.clear();
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
}
