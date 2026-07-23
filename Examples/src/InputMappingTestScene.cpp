#include "InputMappingTestScene.h"

#include <ShitEngine/Core/Log.h>
#include <ShitEngine/Component/Behavior.h>

#include <vector>

namespace {
    constexpr const char* kFontPath = "resource/Roboto-Regular.ttf";

    /**
     * @brief Demonstrates unified Input API (raw keys + action/axis mapping)
     *
     * UIText 目前不支持多行，因此用多条 UIText 分行显示。
     */
    class InputMappingDemoBehavior final : public Shit::Behavior {
    public:
        explicit InputMappingDemoBehavior(std::vector<Shit::UIText*> lines)
            : m_lines(std::move(lines)) {}

        void onUpdate() override {
            if (m_lines.size() < 10) return;

            bool jumpDown = Shit::Input::IsActionDown("Jump");
            bool jumpHeld = Shit::Input::IsActionPressed("Jump");
            bool jumpReleased = Shit::Input::IsActionReleased("Jump");

            bool attackDown = Shit::Input::IsActionDown("Attack");
            bool attackHeld = Shit::Input::IsActionPressed("Attack");
            bool attackReleased = Shit::Input::IsActionReleased("Attack");

            bool sprintHeld = Shit::Input::IsActionPressed("Sprint");
            bool interactDown = Shit::Input::IsActionDown("Interact");
            bool menuDown = Shit::Input::IsActionDown("Menu");

            float h = Shit::Input::GetAxis("Horizontal");
            float v = Shit::Input::GetAxis("Vertical");

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

        std::vector<Shit::UIText*> m_lines;
    };

    Shit::UIText* makeLine(Shit::Scene* scene, Shit::GameObject* canvas,
        const char* name, float yOffset, float fontSize = 18.0f)
    {
        auto* go = scene->createGameObject(name);
        go->setParent(canvas);
        auto* tf = go->addComponent<Shit::UITransform>(0.0f, yOffset, 720.0f, 28.0f);
        tf->setAnchorMin({ 0.5f, 0.5f });
        tf->setAnchorMax({ 0.5f, 0.5f });
        tf->setAnchoredPosition({ 0.0f, yOffset });

        auto* text = go->addComponent<Shit::UIText>("", kFontPath, fontSize);
        text->setColor(Shit::Color{ 220, 220, 220, 255 });
        text->setAnchor(Shit::UIText::TextAnchor::Left);
        return text;
    }
} // namespace

std::unique_ptr<Shit::Scene> createInputMappingTestScene() {
    auto scene = std::make_unique<Shit::Scene>("InputMappingTest");
    scene->init();

    auto* canvas = scene->createGameObject("Canvas");
    canvas->addComponent<Shit::UITransform>(
        0.0f, 0.0f,
        static_cast<float>(Shit::Renderer::GetLogicalWidth()),
        static_cast<float>(Shit::Renderer::GetLogicalHeight()));
    canvas->addComponent<Shit::UICanvas>();

    // Title
    {
        auto* titleGO = scene->createGameObject("Title");
        titleGO->setParent(canvas);
        auto* tf = titleGO->addComponent<Shit::UITransform>(0.0f, 0.0f, 520.0f, 48.0f);
        tf->setAnchorMin({ 0.5f, 0.9f });
        tf->setAnchorMax({ 0.5f, 0.9f });
        auto* text = titleGO->addComponent<Shit::UIText>("Input System Test", kFontPath, 32.0f);
        text->setColor(Shit::Color{ 240, 240, 240, 255 });
    }

    // Status lines (UIText is single-line)
    std::vector<Shit::UIText*> lines;
    lines.push_back(makeLine(scene.get(), canvas, "L0", -40.0f, 20.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L1", -10.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L2",  20.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L3",  50.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L4",  80.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L5", 110.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L6", 150.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L7", 180.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L8", 220.0f, 16.0f));
    lines.push_back(makeLine(scene.get(), canvas, "L9", 248.0f, 16.0f));

    auto* demoGO = scene->createGameObject("InputDemo");
    demoGO->setParent(canvas);
    demoGO->addComponent<InputMappingDemoBehavior>(std::move(lines));

    return scene;
}
