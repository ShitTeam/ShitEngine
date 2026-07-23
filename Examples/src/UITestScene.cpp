#include "UITestScene.h"

#include <ShitEngine/Core/Log.h>

namespace {
    // Asset paths relative to the executable working directory
    constexpr const char* kFontPath   = "resource/Roboto-Regular.ttf";
    constexpr const char* kButtonTex = "resource/grass_side.png";
}

std::unique_ptr<Shit::Scene> createUITestScene() {
    auto scene = std::make_unique<Shit::Scene>("UITest");
    scene->init();

    // ── Canvas ──
    auto* canvas = scene->createGameObject("Canvas");
    canvas->addComponent<Shit::UITransform>(0.0f, 0.0f,
        static_cast<float>(Shit::Renderer::GetLogicalWidth()),
        static_cast<float>(Shit::Renderer::GetLogicalHeight()));
    canvas->addComponent<Shit::UICanvas>();

    // ── Title ──
    {
        auto* title = scene->createGameObject("Title");
        title->setParent(canvas);
        auto* tf = title->addComponent<Shit::UITransform>(0.0f, 0.0f, 360.0f, 60.0f);
        tf->setAnchorMin({ 0.5f, 0.8f });
        tf->setAnchorMax({ 0.5f, 0.8f });

        auto* text = title->addComponent<Shit::UIText>("ShitEngine UI Test", kFontPath, 36.0f);
        text->setColor(Shit::Color{ 240, 240, 240, 255 });
    }

    // ── Hint text (dynamic via setText) ──
    Shit::UIText* hintTextRef = nullptr;
    {
        auto* hintGO = scene->createGameObject("Hint");
        hintGO->setParent(canvas);
        auto* tf = hintGO->addComponent<Shit::UITransform>(0.0f, -40.0f, 600.0f, 32.0f);
        tf->setAnchorMin({ 0.5f, 0.5f });
        tf->setAnchorMax({ 0.5f, 0.5f });

        hintTextRef = hintGO->addComponent<Shit::UIText>("Click the button, count = 0", kFontPath, 20.0f);
        hintTextRef->setColor(Shit::Color{ 200, 200, 200, 255 });
    }

    // ── Main button (hover/pressed colors + onClick) ──
    static int clickCount = 0;
    {
        auto* buttonGO = scene->createGameObject("StartButton");
        buttonGO->setParent(canvas);
        auto* tf = buttonGO->addComponent<Shit::UITransform>(0.0f, 30.0f, 240.0f, 64.0f);
        tf->setAnchorMin({ 0.5f, 0.5f });
        tf->setAnchorMax({ 0.5f, 0.5f });
        tf->setAnchoredPosition({ 0.0f, 30.0f });

        auto* image = buttonGO->addComponent<Shit::UIImage>(kButtonTex);
        image->setColor(Shit::Color{ 120, 180, 255, 255 });

        auto* button = buttonGO->addComponent<Shit::UIButton>();
        Shit::UIButton::ColorBlock colors;
        colors.normalColor      = Shit::Color{ 120, 180, 255, 255 };
        colors.highlightedColor = Shit::Color{ 200, 230, 255, 255 };
        colors.pressedColor     = Shit::Color{  80, 130, 200, 255 };
        colors.disabledColor    = Shit::Color{ 128, 128, 128, 128 };
        button->setColors(colors);

        button->setOnClick([hintTextRef]() {
            ++clickCount;
            ST_INFO("Button clicked! count = {}", clickCount);
            if (hintTextRef) {
                hintTextRef->setText("Button clicked " + std::to_string(clickCount) + " times");
            }
        });
    }

    // ── Left stretch panel (anchorMin ≠ anchorMax) ──
    {
        auto* panel = scene->createGameObject("SidePanel");
        panel->setParent(canvas);
        auto* tf = panel->addComponent<Shit::UITransform>(0.0f, 0.0f, 100.0f, 100.0f);
        tf->setAnchorMin({ 0.0f, 0.0f });
        tf->setAnchorMax({ 0.25f, 1.0f });
        tf->setAnchoredPosition({ 8.0f, 0.0f });

        auto* image = panel->addComponent<Shit::UIImage>(kButtonTex);
        image->setColor(Shit::Color{ 60, 60, 90, 200 });

        // Grandchild: resolves parent rect through two levels up to Canvas
        auto* label = scene->createGameObject("SidePanelLabel");
        label->setParent(panel);
        auto* ltf = label->addComponent<Shit::UITransform>(0.0f, 0.0f, 200.0f, 28.0f);
        ltf->setAnchorMin({ 0.5f, 1.0f });
        ltf->setAnchorMax({ 0.5f, 1.0f });
        ltf->setAnchoredPosition({ 0.0f, -10.0f });

        auto* labelText = label->addComponent<Shit::UIText>("Panel (stretch)", kFontPath, 18.0f);
        labelText->setColor(Shit::Color{ 230, 230, 230, 255 });
    }

    // ── Single-line text input ──
    {
        auto* inputGO = scene->createGameObject("InputBox");
        inputGO->setParent(canvas);
        auto* tf = inputGO->addComponent<Shit::UITransform>(0.0f, 0.0f, 400.0f, 48.0f);
        tf->setAnchorMin({ 0.5f, 0.5f });
        tf->setAnchorMax({ 0.5f, 0.5f });
        tf->setAnchoredPosition({ 0.0f, -80.0f });

        // Background image
        auto* bg = inputGO->addComponent<Shit::UIImage>(kButtonTex);
        bg->setColor(Shit::Color{ 200, 200, 200, 200 });

        auto* input = inputGO->addComponent<Shit::UITextBox>();
        input->setFontPath(kFontPath);
        input->setFontSize(22.0f);
        input->setPlaceholder("Type here...");
        input->setTextColor(Shit::Color{ 30, 30, 30, 255 });
        input->setPlaceholderColor(Shit::Color{ 140, 140, 140, 255 });
        input->setCharacterLimit(20);
    }

    // ── Input label ──
    {
        auto* labelGO = scene->createGameObject("InputLabel");
        labelGO->setParent(canvas);
        auto* tf = labelGO->addComponent<Shit::UITransform>(0.0f, -120.0f, 200.0f, 24.0f);
        tf->setAnchorMin({ 0.5f, 0.5f });
        tf->setAnchorMax({ 0.5f, 0.5f });
        tf->setAnchoredPosition({ 0.0f, -120.0f });

        auto* label = labelGO->addComponent<Shit::UIText>("Text Input:", kFontPath, 18.0f);
        label->setColor(Shit::Color{ 180, 180, 180, 255 });
    }

    return scene;
}
