#pragma once
// ═══════════════════════════════════════════════════════════════
// 示例插件的「脚本」—— 全部为 SHIT_REFLECT 行为组件
// ═══════════════════════════════════════════════════════════════
// 场景内容不再由代码搭建（改由 .scene 数据驱动），这些类作为脚本库：
// 引擎 / 编辑器经 TypeRegistry 实例化、以反射字段配置；
// .scene 通过字段序列化持久化配置（如行名列表、重力值、按钮对象名）。
//
// 约定：
//   - 必须 SHIT_REFLECT(BlackList) + SHIT_REFLECT_BODY（反射工厂要求默认构造）
//   - 配置字段（会被序列化）走 setter 赋值；指向其它对象的引用一律用「对象名
//     字符串」在 onStart 时解析（指针不可序列化）
// ═══════════════════════════════════════════════════════════════

#include <ShitEngine.h>

#include <sstream>
#include <string>
#include <vector>

// ── 场景内按名字解析对象/组件的公共工具 ────────────────────

namespace example_helpers {

inline Shit::GameObject* findGameObject(Shit::Scene* scene, const std::string& name) {
    if (!scene || name.empty()) return nullptr;
    for (auto& go : scene->getGameObjects()) {
        if (go->getName() == name) return go.get();
    }
    return nullptr;
}

inline Shit::Scene* ownerScene(Shit::Behavior* behavior) {
    return behavior->getOwner() ? behavior->getOwner()->getScene() : nullptr;
}

/// 「L0,L1,L2」→ UIText 列表（按对象名解析）
inline std::vector<Shit::UIText*> resolveTextLines(Shit::Scene* scene, const std::string& names) {
    std::vector<Shit::UIText*> lines;
    if (!scene) return lines;
    std::stringstream ss(names);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        if (auto* go = findGameObject(scene, item)) {
            if (auto* t = go->getComponent<Shit::UIText>()) lines.push_back(t);
        }
    }
    return lines;
}

} // namespace example_helpers

// ═══════════════════════════════════════════════════════════════
// GravityConfig —— 把物理重力变成可序列化的场景数据
// ═══════════════════════════════════════════════════════════════

class SHIT_REFLECT(BlackList) GravityConfig : public Shit::Behavior {
    SHIT_REFLECT_BODY(GravityConfig)
public:
    GravityConfig() = default;
    void setGravity(const Shit::Vector2& g) { m_gravity = g; }

    void onStart() override {
        if (auto* scene = example_helpers::ownerScene(this)) {
            if (auto* phys = scene->getSystem<Shit::PhysicsSystem2D>())
                phys->setGravity(m_gravity);
        }
    }

private:
    SHIT_META(({.displayName = "Gravity", .tooltip = "物理世界重力（像素/秒²）"}))
    Shit::Vector2 m_gravity{ 0.0f, 320.0f };
};

// ═══════════════════════════════════════════════════════════════
// ButtonClickDemo —— UIButton 点击 → 更新提示文本（替换原 lambda 不可序列化问题）
// ═══════════════════════════════════════════════════════════════

class SHIT_REFLECT(BlackList) ButtonClickDemo : public Shit::Behavior {
    SHIT_REFLECT_BODY(ButtonClickDemo)
public:
    ButtonClickDemo() = default;
    void setButtonName(const std::string& n) { m_buttonName = n; }
    void setHintName(const std::string& n) { m_hintName = n; }

    void onStart() override {
        Shit::Scene* scene = example_helpers::ownerScene(this);
        Shit::UIButton* button = nullptr;
        if (auto* go = example_helpers::findGameObject(scene, m_buttonName))
            button = go->getComponent<Shit::UIButton>();
        Shit::UIText* hint = nullptr;
        if (auto* go = example_helpers::findGameObject(scene, m_hintName))
            hint = go->getComponent<Shit::UIText>();
        if (!button) {
            ST_WARN("[ButtonClickDemo] 找不到按钮对象 '{}'", m_buttonName);
            return;
        }
        // 演示用：闭包捕获 this（行为与其 GameObject 同生命周期，示例场景内安全）
        button->setOnClick([this, hint]() {
            ++m_clickCount;
            ST_INFO("Button clicked! count = {}", m_clickCount);
            if (hint) hint->setText("Button clicked " + std::to_string(m_clickCount) + " times");
        });
    }

private:
    SHIT_META(({.displayName = "按钮对象名"}))
    std::string m_buttonName;
    SHIT_META(({.displayName = "提示文本对象名"}))
    std::string m_hintName;
    SHIT_META(({.displayName = "点击数", .readOnly = true}))
    int m_clickCount = 0;
};

// ═══════════════════════════════════════════════════════════════
// InputLineDemo —— 输入系统动作/轴状态演示（最初为 InputMappingDemoBehavior）
// ═══════════════════════════════════════════════════════════════

class SHIT_REFLECT(BlackList) InputLineDemo : public Shit::Behavior {
    SHIT_REFLECT_BODY(InputLineDemo)
public:
    InputLineDemo() = default;
    void setLineNames(const std::string& names) { m_lineNames = names; }

    void onStart() override {
        m_lines = example_helpers::resolveTextLines(example_helpers::ownerScene(this), m_lineNames);
    }

    void onUpdate() override {
        if (m_lines.size() < 10) return;

        const bool jumpDown    = Shit::Input::IsActionDown("Jump");
        const bool jumpHeld    = Shit::Input::IsActionPressed("Jump");
        const bool jumpReleased = Shit::Input::IsActionReleased("Jump");

        const bool attackDown    = Shit::Input::IsActionDown("Attack");
        const bool attackHeld    = Shit::Input::IsActionPressed("Attack");
        const bool attackReleased = Shit::Input::IsActionReleased("Attack");

        const bool sprintHeld  = Shit::Input::IsActionPressed("Sprint");
        const bool interactDown = Shit::Input::IsActionDown("Interact");
        const bool menuDown    = Shit::Input::IsActionDown("Menu");

        const float h = Shit::Input::GetAxis("Horizontal");
        const float v = Shit::Input::GetAxis("Vertical");

        m_lines[0]->setText("=== Input Test (Action / Axis) ===");
        m_lines[1]->setText(std::string("Jump:     ") + actionState(jumpDown, jumpHeld, jumpReleased));
        m_lines[2]->setText(std::string("Attack:   ") + actionState(attackDown, attackHeld, attackReleased));
        m_lines[3]->setText(std::string("Sprint:   ") + (sprintHeld ? "[Held]" : "[---]"));
        m_lines[4]->setText(std::string("Interact: ") + (interactDown ? "[Down]" : "[---]"));
        m_lines[5]->setText(std::string("Menu:     ") + (menuDown ? "[Down]" : "[---]"));
        m_lines[6]->setText(std::string("Horizontal (A/D): ") + axisStr(h));
        m_lines[7]->setText(std::string("Vertical   (S/W): ") + axisStr(v));
        m_lines[8]->setText("Binds: Space / J|E / Left Shift / F / Esc");
        m_lines[9]->setText("Edit settings.json → inputMappings to rebind");
    }

private:
    static std::string actionState(bool down, bool held, bool released) {
        if (released) return "[Released]";
        if (held) return "[Held]";
        if (down) return "[Down]";
        return "[---]";
    }

    static std::string axisStr(float val) {
        if (val > 0.5f) return "+1";
        if (val < -0.5f) return "-1";
        return " 0";
    }

    SHIT_META(({.displayName = "行对象名（L0,L1,...）", .tooltip = "解析多条 UIText 对象用于显示状态"}))
    std::string m_lineNames;
    // 容器不可序列化，必须显式 Disable：libclang 会把 std::vector<...> 退化为 "int"
    // 注册（BlackList 无条件收录），运行时按 4 字节读写 24 字节 vector 会损坏内存
    // （Prefab/检查器另有 size 兜底防御，这里在源头排除）
    SHIT_META(Disable)
    std::vector<Shit::UIText*> m_lines;  ///< 运行时按名解析（容器类型不可序列化）
};