#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Animation/Animator.h"

#include "ShitEngine/Render/Animation.h"
#include "ShitEngine/Component/SpriteRenderer.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace Shit {

    // ═══════════════════════════════════════════════════════════
    // JSON 序列化辅助
    // ═══════════════════════════════════════════════════════════

    static nlohmann::json parameterToJson(const AnimatorParameter& p) {
        nlohmann::json j;
        j["name"] = p.name;
        j["type"] = static_cast<int>(p.type);
        j["floatValue"] = p.floatValue;
        j["boolValue"] = p.boolValue;
        return j;
    }
    static bool parameterFromJson(const nlohmann::json& j, AnimatorParameter& p) {
        if (!j.is_object()) return false;
        try {
            if (j.contains("name") && j["name"].is_string()) p.name = j["name"].get<std::string>();
            if (j.contains("type") && j["type"].is_number_integer()) p.type = static_cast<AnimatorParamType>(j["type"].get<int>());
            if (j.contains("floatValue") && j["floatValue"].is_number()) p.floatValue = j["floatValue"].get<float>();
            if (j.contains("boolValue") && j["boolValue"].is_boolean()) p.boolValue = j["boolValue"].get<bool>();
            return true;
        } catch (...) { return false; }
    }

    static nlohmann::json conditionToJson(const AnimatorTransitionCondition& c) {
        nlohmann::json j;
        j["parameter"] = c.parameter;
        j["type"] = static_cast<int>(c.type);
        j["threshold"] = c.threshold;
        j["boolValue"] = c.boolValue;
        return j;
    }
    static bool conditionFromJson(const nlohmann::json& j, AnimatorTransitionCondition& c) {
        if (!j.is_object()) return false;
        try {
            if (j.contains("parameter") && j["parameter"].is_string()) c.parameter = j["parameter"].get<std::string>();
            if (j.contains("type") && j["type"].is_number_integer()) c.type = static_cast<AnimatorConditionType>(j["type"].get<int>());
            if (j.contains("threshold") && j["threshold"].is_number()) c.threshold = j["threshold"].get<float>();
            if (j.contains("boolValue") && j["boolValue"].is_boolean()) c.boolValue = j["boolValue"].get<bool>();
            return true;
        } catch (...) { return false; }
    }

    static nlohmann::json stateToJson(const AnimatorState& s) {
        nlohmann::json j;
        j["name"] = s.name;
        j["isEntry"] = s.isEntry;
        j["graphX"] = s.graphX;
        j["graphY"] = s.graphY;
        j["clip"] = s.clip.toJson();
        return j;
    }
    static bool stateFromJson(const nlohmann::json& j, AnimatorState& s) {
        if (!j.is_object()) return false;
        try {
            if (j.contains("name") && j["name"].is_string()) s.name = j["name"].get<std::string>();
            if (j.contains("isEntry") && j["isEntry"].is_boolean()) s.isEntry = j["isEntry"].get<bool>();
            if (j.contains("graphX") && j["graphX"].is_number()) s.graphX = j["graphX"].get<float>();
            if (j.contains("graphY") && j["graphY"].is_number()) s.graphY = j["graphY"].get<float>();
            if (j.contains("clip") && j["clip"].is_object()) s.clip.fromJson(j["clip"]);
            return true;
        } catch (...) { return false; }
    }

    static nlohmann::json transitionToJson(const AnimatorTransition& t) {
        nlohmann::json j;
        j["fromState"] = t.fromState;
        j["toState"] = t.toState;
        j["exitTime"] = t.exitTime;
        nlohmann::json conds = nlohmann::json::array();
        for (const auto& c : t.conditions) conds.push_back(conditionToJson(c));
        j["conditions"] = conds;
        return j;
    }
    static bool transitionFromJson(const nlohmann::json& j, AnimatorTransition& t) {
        if (!j.is_object()) return false;
        try {
            if (j.contains("fromState") && j["fromState"].is_number_integer()) t.fromState = j["fromState"].get<int>();
            if (j.contains("toState") && j["toState"].is_number_integer()) t.toState = j["toState"].get<int>();
            if (j.contains("exitTime") && j["exitTime"].is_number()) t.exitTime = j["exitTime"].get<float>();
            if (j.contains("conditions") && j["conditions"].is_array()) {
                t.conditions.clear();
                for (const auto& cj : j["conditions"]) {
                    AnimatorTransitionCondition c;
                    if (conditionFromJson(cj, c)) t.conditions.push_back(std::move(c));
                }
            }
            return true;
        } catch (...) { return false; }
    }

    // ═══════════════════════════════════════════════════════════

    Animator::Animator() = default;
    Animator::~Animator() = default;

    void Animator::onAttach() {
        Behavior::onAttach();
    }

    void Animator::onStart() {
        if (m_states.empty()) return;
        // 优先进入入口状态；无入口则第一个
        int entry = -1;
        for (size_t i = 0; i < m_states.size(); ++i)
            if (m_states[i].isEntry) { entry = static_cast<int>(i); break; }
        if (entry < 0) entry = 0;
        enterState(entry);
    }

    void Animator::onUpdate() {
        if (!m_isPlaying || m_currentState < 0) return;

        // 1. 推进当前状态动画
        if (m_currentAnimation) {
            m_animTime += Time::GetDeltaTime();
            if (!m_currentAnimation->isLooping()) {
                float total = static_cast<float>(m_currentAnimation->getFrameCount())
                              * m_currentAnimation->getDuration();
                if (total > 0.0f && m_animTime >= total)
                    m_animTime = total;
            }
            applyCurrentFrame();
        }

        // 2. 求值转换
        if (evaluateTransitions()) {
            // 已切换（enterState 内部重建 Animation 并写回首帧）
            return;
        }
    }

    void Animator::onDestroy() {
        Behavior::onDestroy();
        m_currentState = -1;
        m_currentAnimation.reset();
        m_isPlaying = false;
        m_states.clear();
        m_params.clear();
        m_transitions.clear();
    }

    void Animator::onAfterDeserialize() {
        parseData();
        m_currentStateDisplay = m_currentState >= 0 && m_currentState < static_cast<int>(m_states.size())
                                  ? m_states[static_cast<size_t>(m_currentState)].name : "";
        m_playingDisplay = m_isPlaying;
    }

    void Animator::onFieldChanged(const std::string& fieldName) {
        if (fieldName == "m_animatorData")
            parseData();
    }

    void Animator::parseData() {
        m_states.clear();
        m_params.clear();
        m_transitions.clear();
        if (m_animatorData.empty()) return;
        try {
            nlohmann::json root = nlohmann::json::parse(m_animatorData);
            if (root.contains("states") && root["states"].is_array())
                for (const auto& sj : root["states"]) {
                    AnimatorState s;
                    if (stateFromJson(sj, s)) m_states.push_back(std::move(s));
                }
            if (root.contains("params") && root["params"].is_array())
                for (const auto& pj : root["params"]) {
                    AnimatorParameter p;
                    if (parameterFromJson(pj, p)) m_params.push_back(std::move(p));
                }
            if (root.contains("transitions") && root["transitions"].is_array())
                for (const auto& tj : root["transitions"]) {
                    AnimatorTransition t;
                    if (transitionFromJson(tj, t)) m_transitions.push_back(std::move(t));
                }
        } catch (const std::exception& e) {
            ST_CORE_WARN("Animator: 解析 m_animatorData 失败: {}", e.what());
        }
        // 保持默认播放状态（运行时在 onStart 进入）
        m_currentState = -1;
    }

    void Animator::notifyDataChanged() {
        ++m_dataGeneration;
        syncData();
    }

    void Animator::syncData() {
        nlohmann::json root;
        nlohmann::json states = nlohmann::json::array();
        for (const auto& s : m_states) states.push_back(stateToJson(s));
        nlohmann::json params = nlohmann::json::array();
        for (const auto& p : m_params) params.push_back(parameterToJson(p));
        nlohmann::json trans = nlohmann::json::array();
        for (const auto& t : m_transitions) trans.push_back(transitionToJson(t));
        root["states"] = states;
        root["params"] = params;
        root["transitions"] = trans;
        m_animatorData = root.dump();
    }

    // ═══════════════════════════════════════════════════════════
    // 参数驱动
    // ═══════════════════════════════════════════════════════════

    int Animator::findParam(const std::string& name) const {
        for (size_t i = 0; i < m_params.size(); ++i)
            if (m_params[i].name == name) return static_cast<int>(i);
        return -1;
    }

    void Animator::setFloat(const std::string& name, float value) {
        int i = findParam(name);
        if (i < 0) return;
        if (m_params[static_cast<size_t>(i)].type != AnimatorParamType::Float) return;
        m_params[static_cast<size_t>(i)].floatValue = value;
    }

    void Animator::setBool(const std::string& name, bool value) {
        int i = findParam(name);
        if (i < 0) return;
        if (m_params[static_cast<size_t>(i)].type != AnimatorParamType::Bool) return;
        m_params[static_cast<size_t>(i)].boolValue = value;
    }

    void Animator::setTrigger(const std::string& name) {
        int i = findParam(name);
        if (i < 0) return;
        if (m_params[static_cast<size_t>(i)].type != AnimatorParamType::Trigger) return;
        m_params[static_cast<size_t>(i)].boolValue = true;  // 触发标志；求值后由 checkTransition 消耗
    }

    // ═══════════════════════════════════════════════════════════
    // 状态/参数/转换管理
    // ═══════════════════════════════════════════════════════════

    const AnimatorState* Animator::stateAt(int index) const {
        if (index < 0 || index >= static_cast<int>(m_states.size())) return nullptr;
        return &m_states[static_cast<size_t>(index)];
    }

    const std::string& Animator::currentStateName() const {
        static const std::string kEmpty;
        const AnimatorState* s = stateAt(m_currentState);
        return s ? s->name : kEmpty;
    }

    const AnimatorParameter* Animator::paramAt(int index) const {
        if (index < 0 || index >= static_cast<int>(m_params.size())) return nullptr;
        return &m_params[static_cast<size_t>(index)];
    }

    const AnimatorTransition* Animator::transitionAt(int index) const {
        if (index < 0 || index >= static_cast<int>(m_transitions.size())) return nullptr;
        return &m_transitions[static_cast<size_t>(index)];
    }

    int Animator::addParam(const std::string& name, AnimatorParamType type) {
        std::string base = name.empty() ? "Param" : name;
        std::string candidate = base;
        int n = 1;
        while (findParam(candidate) >= 0) candidate = base + " (" + std::to_string(n++) + ")";
        AnimatorParameter p;
        p.name = candidate;
        p.type = type;
        m_params.push_back(std::move(p));
        notifyDataChanged();
        return static_cast<int>(m_params.size()) - 1;
    }

    bool Animator::setParam(int index, const AnimatorParameter& param) {
        if (index < 0 || index >= static_cast<int>(m_params.size())) return false;
        m_params[static_cast<size_t>(index)] = param;
        notifyDataChanged();
        return true;
    }

    bool Animator::removeParam(int index) {
        if (index < 0 || index >= static_cast<int>(m_params.size())) return false;
        const std::string removedName = m_params[static_cast<size_t>(index)].name;
        m_params.erase(m_params.begin() + index);
        // 清理引用该参数的转换条件
        for (auto& t : m_transitions) {
            t.conditions.erase(
                std::remove_if(t.conditions.begin(), t.conditions.end(),
                               [&](const AnimatorTransitionCondition& c) { return c.parameter == removedName; }),
                t.conditions.end());
        }
        notifyDataChanged();
        return true;
    }

    int Animator::addState(const std::string& name) {
        std::string base = name.empty() ? "State" : name;
        std::string candidate = base;
        int n = 1;
        bool exists = true;
        while (exists) {
            exists = false;
            for (const auto& s : m_states) if (s.name == candidate) { exists = true; break; }
            if (exists) candidate = base + " (" + std::to_string(n++) + ")";
        }
        AnimatorState s;
        s.name = candidate;
        if (m_states.empty()) s.isEntry = true;  // 首状态自动为入口
        m_states.push_back(std::move(s));
        notifyDataChanged();
        return static_cast<int>(m_states.size()) - 1;
    }

    bool Animator::setState(int index, const AnimatorState& state) {
        if (index < 0 || index >= static_cast<int>(m_states.size())) return false;
        m_states[static_cast<size_t>(index)] = state;
        // 若正在播放且是当前状态，重新进入以反映剪辑变化
        if (m_currentState == index && m_isPlaying) enterState(index);
        notifyDataChanged();
        return true;
    }

    bool Animator::removeState(int index) {
        if (index < 0 || index >= static_cast<int>(m_states.size())) return false;
        // 移除的是当前状态 → 停止
        if (m_currentState == index) {
            m_currentState = -1;
            m_currentAnimation.reset();
            m_isPlaying = false;
        } else if (m_currentState > index) {
            --m_currentState;  // 索引左移
        }
        m_states.erase(m_states.begin() + index);
        // 清理引用该索引的转换（并修正更大索引的 from/to）
        for (auto it = m_transitions.begin(); it != m_transitions.end();) {
            if (it->fromState == index || it->toState == index) { it = m_transitions.erase(it); continue; }
            if (it->fromState > index) --it->fromState;
            if (it->toState > index) --it->toState;
            ++it;
        }
        notifyDataChanged();
        return true;
    }

    int Animator::addTransition(int fromState, int toState) {
        if (toState < 0 || toState >= static_cast<int>(m_states.size())) return -1;
        AnimatorTransition t;
        t.fromState = fromState;
        t.toState = toState;
        m_transitions.push_back(std::move(t));
        notifyDataChanged();
        return static_cast<int>(m_transitions.size()) - 1;
    }

    bool Animator::setTransition(int index, const AnimatorTransition& transition) {
        if (index < 0 || index >= static_cast<int>(m_transitions.size())) return false;
        m_transitions[static_cast<size_t>(index)] = transition;
        notifyDataChanged();
        return true;
    }

    bool Animator::removeTransition(int index) {
        if (index < 0 || index >= static_cast<int>(m_transitions.size())) return false;
        m_transitions.erase(m_transitions.begin() + index);
        notifyDataChanged();
        return true;
    }

    // ═══════════════════════════════════════════════════════════
    // 运行时
    // ═══════════════════════════════════════════════════════════

    void Animator::enterState(int index) {
        const AnimatorState* s = stateAt(index);
        if (!s) return;
        m_currentState = index;
        m_animTime = 0.0f;
        m_isPlaying = true;
        m_currentStateDisplay = s->name;
        m_playingDisplay = true;

        const AnimationClip& clip = s->clip;
        if (clip.frames.empty()) {
            m_currentAnimation.reset();
            return;
        }
        SpriteSheet sheet(clip.rows, clip.cols, clip.frameWidth, clip.frameHeight, clip.margin, clip.spacing);
        auto anim = std::make_unique<Animation>(clip.duration, clip.loop);
        for (int idx : clip.frames)
            anim->addFrame(sheet.getFrameRect(idx));
        m_currentAnimation = std::move(anim);
        applyCurrentFrame();
    }

    void Animator::setCurrentState(int index) {
        if (index >= 0 && index < static_cast<int>(m_states.size()))
            enterState(index);
    }

    bool Animator::checkTransition(const AnimatorTransition& t) const {
        for (const auto& c : t.conditions) {
            int pi = findParam(c.parameter);
            const AnimatorParameter* p = pi >= 0 ? &m_params[static_cast<size_t>(pi)] : nullptr;
            switch (c.type) {
                case AnimatorConditionType::FloatGt:
                    if (!p || p->type != AnimatorParamType::Float || !(p->floatValue > c.threshold)) return false;
                    break;
                case AnimatorConditionType::FloatLt:
                    if (!p || p->type != AnimatorParamType::Float || !(p->floatValue < c.threshold)) return false;
                    break;
                case AnimatorConditionType::FloatEq:
                    if (!p || p->type != AnimatorParamType::Float || std::fabs(p->floatValue - c.threshold) > 0.001f) return false;
                    break;
                case AnimatorConditionType::Bool:
                    if (!p || p->type != AnimatorParamType::Bool || p->boolValue != c.boolValue) return false;
                    break;
                case AnimatorConditionType::Trigger:
                    if (!p || p->type != AnimatorParamType::Trigger || !p->boolValue) return false;
                    break;
            }
        }
        return true;
    }

    bool Animator::evaluateTransitions() {
        if (m_currentState < 0 || m_transitions.empty()) return false;
        // 先求值当前状态的精确转换，再求值"任意状态"转换（from=-1）
        for (const auto& t : m_transitions) {
            if (t.fromState != m_currentState) continue;
            if (t.toState < 0 || t.toState >= static_cast<int>(m_states.size())) continue;
            if (checkTransition(t)) {
                // 消耗本转换用到的 trigger 参数
                for (const auto& c : t.conditions)
                    if (c.type == AnimatorConditionType::Trigger) {
                        int pi = findParam(c.parameter);
                        if (pi >= 0) m_params[static_cast<size_t>(pi)].boolValue = false;
                    }
                enterState(t.toState);
                return true;
            }
        }
        for (const auto& t : m_transitions) {
            if (t.fromState != -1) continue;
            if (t.toState == m_currentState || t.toState < 0 || t.toState >= static_cast<int>(m_states.size())) continue;
            if (checkTransition(t)) {
                for (const auto& c : t.conditions)
                    if (c.type == AnimatorConditionType::Trigger) {
                        int pi = findParam(c.parameter);
                        if (pi >= 0) m_params[static_cast<size_t>(pi)].boolValue = false;
                    }
                enterState(t.toState);
                return true;
            }
        }
        return false;
    }

    void Animator::applyCurrentFrame() {
        if (!m_currentAnimation || m_currentAnimation->getFrameCount() == 0) return;
        SDL_FRect frame = m_currentAnimation->getFrame(m_animTime);
        auto* owner = getOwner();
        if (!owner) return;
        if (auto* sprite = owner->getComponent<SpriteRenderer>())
            sprite->setSourceRect(frame);
    }
}
