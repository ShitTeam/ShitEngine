#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Animation/AnimationClip.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Shit {

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
        j["frames"] = frames;
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
            if (j.contains("frames") && j["frames"].is_array()) {
                frames.clear();
                for (const auto& f : j["frames"])
                    if (f.is_number_integer()) frames.push_back(f.get<int>());
            }
            if (j.contains("frameDurations") && j["frameDurations"].is_array()) {
                frameDurations.clear();
                for (const auto& d : j["frameDurations"])
                    if (d.is_number()) frameDurations.push_back(d.get<float>());
                // 长度与帧数不匹配则丢弃（避免脏数据影响播放）
                if (frameDurations.size() != frames.size()) frameDurations.clear();
            }
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
}
