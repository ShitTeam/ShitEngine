#pragma once

#include "../Component/Behavior.h"
#include "../Core/Core.h"
#include "../Render/SpriteSheet.h"
#include "AnimationClip.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Shit
{
    class Animation;

    /// 动画参数类型（状态机条件变量）；以 int 存 JSON 载体，无需反射
    enum class AnimatorParamType : int {
        Float = 0,     ///< 浮点（比较 threshold）
        Bool = 1,      ///< 布尔（比较 boolValue）
        Trigger = 2,   ///< 触发器（一次性触发，转换求值后消耗）
    };

    /// 动画参数（可序列化）
    struct SHIT_API AnimatorParameter {
        std::string name;
        AnimatorParamType type = AnimatorParamType::Float;
        float floatValue = 0.0f;   ///< Float 当前值
        bool boolValue = false;    ///< Bool 当前值 / Trigger 触发标志
    };

    /// 条件类型（转换求值）；以 int 存 JSON 载体，无需反射
    enum class AnimatorConditionType : int {
        FloatGt = 0,   ///< float > threshold
        FloatLt = 1,   ///< float < threshold
        FloatEq = 2,   ///< |float - threshold| < 0.001
        Bool = 3,      ///< bool == boolValue
        Trigger = 4,   ///< trigger 已触发
    };

    /// 转换的单个条件（引用参数名）
    struct SHIT_API AnimatorTransitionCondition {
        std::string parameter;               ///< 参数名
        AnimatorConditionType type = AnimatorConditionType::FloatGt;
        float threshold = 0.0f;              ///< Float 阈值
        bool boolValue = false;              ///< Bool 期望值
    };

    /// 状态机节点（可序列化）
    struct SHIT_API AnimatorState {
        std::string name;         ///< 状态名
        AnimationClip clip;       ///< 本状态播放的剪辑（内嵌数据）
        bool isEntry = false;     ///< 是否入口状态（onStart 优先进入）
        float graphX = 0.0f;      ///< 状态机图节点 X（编辑器布局用，随 .scene 保存）
        float graphY = 0.0f;      ///< 状态机图节点 Y（编辑器布局用，随 .scene 保存）
    };

    /// 状态间转换（可序列化；同一状态可有多条，按序求值）
    struct SHIT_API AnimatorTransition {
        int fromState = -1;                 ///< 源状态索引（-1 = 任意）
        int toState = -1;                   ///< 目标状态索引
        std::vector<AnimatorTransitionCondition> conditions;  ///< 全部满足才切换
        float exitTime = 0.0f;              ///< 保留：切换延迟（秒），当前实现立即切换
    };

    /**
     * @brief 动画状态机 / Animator（P28）
     *
     * 继承 Behavior，由 BehaviorSystem 每帧调用 onUpdate：
     *   1. 推进当前状态的剪辑播放（Animation 对象），并把当前帧源矩形回写到同 GameObject 的 SpriteRenderer；
     *   2. 遍历当前状态（及任意 from=-1）的转换，求值条件；满足则切换到目标状态并重启动画。
     *
     * 条件用参数（setFloat / setBool / setTrigger）驱动——典型场景：角色 idle/run/jump 剪辑
     * 依 speed（float）、grounded（bool）、jump（trigger）切换。
     *
     * 序列化：状态/参数/转换以反射字符串载体 m_animatorData（JSON）随 .scene 落盘，
     * onAfterDeserialize / onFieldChanged 解析重建；编辑器经 addState/setState/... 修改后 syncData()。
     */
    class SHIT_API SHIT_REFLECT(BlackList) Animator : public Behavior {
        SHIT_REFLECT_BODY(Animator)
    public:
        Animator();
        ~Animator() override;

        // ── 生命周期 ──
        void onAttach() override;
        void onStart() override;
        void onUpdate() override;
        void onDestroy() override;
        void onAfterDeserialize() override;
        void onFieldChanged(const std::string& fieldName) override;

        // ── 参数驱动 ──
        void setFloat(const std::string& name, float value);
        void setBool(const std::string& name, bool value);
        void setTrigger(const std::string& name);

        // ── 状态查询 ──
        int stateCount() const { return static_cast<int>(m_states.size()); }
        const AnimatorState* stateAt(int index) const;
        int currentStateIndex() const { return m_currentState; }
        const std::string& currentStateName() const;
        void setCurrentState(int index);   ///< 强制切到指定状态（编辑器预览/调试用）
        /// 立即求值一次转换（编辑器/运行共用；onUpdate 自动调用）
        bool evaluateTransitions();

        // ── 参数管理 ──
        int paramCount() const { return static_cast<int>(m_params.size()); }
        const AnimatorParameter* paramAt(int index) const;
        int addParam(const std::string& name, AnimatorParamType type);
        bool setParam(int index, const AnimatorParameter& param);
        bool removeParam(int index);
        int findParam(const std::string& name) const;

        // ── 状态管理 ──
        int addState(const std::string& name);
        bool setState(int index, const AnimatorState& state);
        bool removeState(int index);       ///< 连带清理引用该状态的转换

        // ── 转换管理 ──
        int transitionCount() const { return static_cast<int>(m_transitions.size()); }
        const AnimatorTransition* transitionAt(int index) const;
        int addTransition(int fromState, int toState);
        bool setTransition(int index, const AnimatorTransition& transition);
        bool removeTransition(int index);

        // ── 序列化载体 ──
        /// 把当前 states/params/transitions 同步到 m_animatorData（编辑器修改后调用）
        void syncData();

        // ── 运行时查询 ──
        bool isPlaying() const { return m_isPlaying; }
        /// 数据代数，每次状态/参数/转换变更后递增（编辑器用于判断是否需要重建状态机图）
        uint64_t getDataGeneration() const { return m_dataGeneration; }

    private:
        /// 进入状态：构建 Animation 并重置时间
        void enterState(int index);
        /// 解析 m_animatorData 到内部容器
        void parseData();
        /// 数据变化统一入口：重建 + 同步载体
        void notifyDataChanged();
        /// 把当前帧源矩形写回 SpriteRenderer
        void applyCurrentFrame();
        /// 检查一条转换是否满足
        bool checkTransition(const AnimatorTransition& t) const;

        // 反射字符串载体（JSON：states / params / transitions）
        SHIT_META(({.displayName = "Animator Data", .tooltip = "状态机序列化载体（JSON），由编辑器维护", .readOnly = true}))
        std::string m_animatorData;

        // 运行时（Disable 不入反射）
        SHIT_META(Disable)
        std::vector<AnimatorState> m_states;
        SHIT_META(Disable)
        std::vector<AnimatorParameter> m_params;
        SHIT_META(Disable)
        std::vector<AnimatorTransition> m_transitions;

        SHIT_META(Disable)
        int m_currentState = -1;
        SHIT_META(Disable)
        std::unique_ptr<Animation> m_currentAnimation;
        SHIT_META(Disable)
        float m_animTime = 0.0f;
        SHIT_META(Disable)
        bool m_isPlaying = false;

        SHIT_META(Disable)
        uint64_t m_dataGeneration = 0;  ///< 数据代数，notifyDataChanged 递增

        // 只读展示字段
        SHIT_META(({.displayName = "Current State", .readOnly = true}))
        std::string m_currentStateDisplay;
        SHIT_META(({.displayName = "Playing", .readOnly = true}))
        bool m_playingDisplay = false;
    };
}
