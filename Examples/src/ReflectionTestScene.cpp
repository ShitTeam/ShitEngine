#include "ReflectionTestScene.h"

#include <ShitEngine/Core/Log.h>
#include <ShitEngine/Reflection/TypeRegistry.h>
#include <ShitEngine/GameObject/Prefab.h>
#include <ShitEngine/Component/TransformComponent.h>
#include <ShitEngine/Component/CameraComponent.h>

#include <string>
#include <sstream>

#include "ReflectionTestTypes.h"

// 类型定义已移至 ReflectionTestTypes.h（供 ReflectionScanner 扫描）
// 类型注册由 plugin_export.cpp → RegisterAllReflectedTypes() 自动完成

namespace {

// ══════════════════════════════════════════════════════
// Behavior — 验证反射功能
// ══════════════════════════════════════════════════════

class ReflectionTestBehavior final : public Shit::Behavior {
public:
    explicit ReflectionTestBehavior(std::vector<Shit::UIText*> lines)
        : m_lines(std::move(lines)) {}

    void onStart() override {
        runReflectionTests();

        // 显示测试摘要
        updateUIText();
    }

    void onUpdate() override {
        // 一帧后不再重复更新
    }

private:
    void runReflectionTests() {
        ST_CORE_INFO("========== Reflection System Tests ==========");

        // ── Test 1: Count ──
        size_t total = Shit::TypeRegistry::Count();
        ST_CORE_INFO("[Count] Registered types: {}", total);
        m_results.push_back(std::string("Total types: ") + std::to_string(total));

        // ── Test 2: Query by name ──
        const auto* tpInfo = Shit::TypeRegistry::Get("TestPlayer");
        if (tpInfo) {
            ST_CORE_INFO("[Get-by-name] Found '{}', size={}, fields={}",
                tpInfo->name, tpInfo->size, tpInfo->fields.size());
            m_results.push_back(std::string("Get(\"TestPlayer\"): ") +
                tpInfo->name + " (" + std::to_string(tpInfo->fields.size()) + " fields)");
        } else {
            m_results.push_back("Get(\"TestPlayer\"): FAILED");
        }

        // ── Test 3: Check base type ──
        if (tpInfo && tpInfo->baseType) {
            ST_CORE_INFO("[BaseType] {} -> {}", tpInfo->name, tpInfo->baseType->name);
            m_results.push_back(std::string("Base: ") + tpInfo->baseType->name);
        }

        // ── Test 4: Check fields ──
        if (tpInfo) {
            for (size_t i = 0; i < tpInfo->fields.size(); ++i) {
                const auto& f = tpInfo->fields[i];
                ST_CORE_INFO("  [Field {}] name={}, offset={}, size={}, type={}",
                    i, f.name, f.offset, f.size, f.typeName);
                std::stringstream ss;
                ss << "  Field: " << f.name << " (" << f.typeName
                << ", off=" << f.offset << ")";
                m_results.push_back(ss.str());
            }
        }

        // ── Test 5: Query by type_index ──
        const auto* tpByType = Shit::TypeRegistry::Get<TestPlayer>();
        if (tpByType) {
            ST_CORE_INFO("[Get<T>()] Found '{}' by type_index", tpByType->name);
            m_results.push_back(std::string("Get<TestPlayer>(): ") + tpByType->name);
        } else {
            m_results.push_back("Get<TestPlayer>(): FAILED");
        }

        // ── Test 6: Query nonexistent type ──
        const auto* bad = Shit::TypeRegistry::Get("NonExistent");
        if (bad == nullptr) {
            ST_CORE_INFO("[NotFound] Get(\"NonExistent\") correctly returned null");
            m_results.push_back("Get(\"NonExistent\"): null (OK)");
        }

        // ── Test 7: Builtin types ──
        const auto* intInfo = Shit::TypeRegistry::Get("int");
        const auto* strInfo = Shit::TypeRegistry::Get("std::string");
        const auto* floatInfo = Shit::TypeRegistry::Get("float");

        if (intInfo && strInfo && floatInfo) {
            ST_CORE_INFO("[Builtins] int={}, std::string={}, float={}",
                intInfo->size, strInfo->size, floatInfo->size);
            m_results.push_back(std::string("Builtins: int(") +
                std::to_string(intInfo->size) + "B), float(" +
                std::to_string(floatInfo->size) + "B), string(" +
                std::to_string(strInfo->size) + "B)");
        } else {
            m_results.push_back("Builtin types: FAILED");
        }

        // ── Test 8: Negative test — builtin type via template ──
        const auto* intByType = Shit::TypeRegistry::Get<int>();
        if (intByType && intByType == intInfo) {
            ST_CORE_INFO("[Get<int>()] matches Get(\"int\") — OK");
            m_results.push_back("Get<int>() == Get(\"int\"): OK");
        } else {
            m_results.push_back("Get<int>() == Get(\"int\"): FAILED");
        }

        // ── Test 9: ForEach — list all types ──
        ST_CORE_INFO("[ForEach] All registered types:");
        Shit::TypeRegistry::ForEach([](const Shit::TypeInfo& info) {
            ST_CORE_INFO("  {}", info.name);
        });
        m_results.push_back(std::string("ForEach: ") +
            std::to_string(Shit::TypeRegistry::Count()) + " types listed");

        // ── Test 10: Prefab Capture → JSON round-trip → instantiate ──
        {
            Shit::Scene* scene = getOwner() ? getOwner()->getScene() : nullptr;
            if (scene) {
                // 源对象：Transform（position/scale 可序列化）+ Camera（worldSize/zoom/priority 可序列化）
                auto* srcGO = scene->createGameObject("PrefabSource");
                auto* srcTf = srcGO->addComponent<Shit::TransformComponent>();
                srcTf->setPosition({ 123.0f, 456.0f });
                srcTf->setScale({ 2.0f, 3.0f });
                auto* srcCam = srcGO->addComponent<Shit::CameraComponent>();
                srcCam->setSize({ 320.0f, 240.0f });
                srcCam->setZoom(1.5f);

                auto prefab = Shit::Prefab::Capture(srcGO);
                ST_CORE_INFO("[Prefab] Captured {} components", prefab.getComponents().size());
                m_results.push_back(std::string("Prefab Capture: ") +
                    std::to_string(prefab.getComponents().size()) + " comps");

                // JSON 序列化 → 反序列化
                auto json = prefab.toJson();
                ST_CORE_INFO("[Prefab] JSON: {}", json.dump());
                auto prefab2 = Shit::Prefab::FromJson(json);
                m_results.push_back(std::string("Prefab JSON round-trip: ") +
                    std::to_string(prefab2.getComponents().size()) + " comps");

                // 实例化并校验字段
                auto* clone = prefab.instantiate(scene, "PrefabClone");
                auto* cloneTf = clone->getComponent<Shit::TransformComponent>();
                auto* cloneCam = clone->getComponent<Shit::CameraComponent>();
                if (cloneTf && cloneTf->getPosition().x == 123.0f && cloneTf->getPosition().y == 456.0f
                    && cloneTf->getScale().x == 2.0f && cloneTf->getScale().y == 3.0f
                    && cloneCam && cloneCam->getSize().x == 320.0f && cloneCam->getSize().y == 240.0f
                    && cloneCam->getZoom() == 1.5f) {
                    ST_CORE_INFO("[Prefab] Clone 字段全部匹配 — OK");
                    m_results.push_back("Prefab Clone fields: OK");
                } else {
                    ST_CORE_ERROR("[Prefab] Clone 字段不匹配");
                    m_results.push_back("Prefab Clone fields: FAILED");
                }
            } else {
                m_results.push_back("Prefab test: no scene");
            }
        }

        ST_CORE_INFO("=============================================");
    }

    void updateUIText() {
        for (size_t i = 0; i < m_lines.size(); ++i) {
            if (i < m_results.size()) {
                m_lines[i]->setText(m_results[i]);
            } else {
                m_lines[i]->setText("");
            }
        }
    }

    std::vector<std::string> m_results;
    std::vector<Shit::UIText*> m_lines;
};

constexpr const char* kFontPath = "resource/Roboto-Regular.ttf";

Shit::UIText* makeLine(Shit::Scene* scene, Shit::GameObject* canvas,
    const char* name, float yOffset, float fontSize = 17.0f)
{
    auto* go = scene->createGameObject(name);
    go->setParent(canvas);
    auto* tf = go->addComponent<Shit::UITransform>(0.0f, yOffset, 780.0f, 24.0f);
    tf->setAnchorMin({ 0.5f, 0.5f });
    tf->setAnchorMax({ 0.5f, 0.5f });
    tf->setAnchoredPosition({ 0.0f, yOffset });

    auto* text = go->addComponent<Shit::UIText>("", kFontPath, fontSize);
    text->setColor(Shit::Color{ 200, 220, 240, 255 });
    text->setAnchor(Shit::UIText::TextAnchor::Left);
    return text;
}

} // anonymous namespace

// ══════════════════════════════════════════════════════
// 场景入口
// ══════════════════════════════════════════════════════

std::unique_ptr<Shit::Scene> createReflectionTestScene() {
    auto scene = std::make_unique<Shit::Scene>("ReflectionTest");
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
        auto* tf = titleGO->addComponent<Shit::UITransform>(0.0f, 0.0f, 640.0f, 42.0f);
        tf->setAnchorMin({ 0.5f, 0.9f });
        tf->setAnchorMax({ 0.5f, 0.9f });
        auto* text = titleGO->addComponent<Shit::UIText>("Reflection System Test", kFontPath, 28.0f);
        text->setColor(Shit::Color{ 240, 240, 240, 255 });
    }

    // Result lines
    std::vector<Shit::UIText*> lines;
    lines.push_back(makeLine(scene.get(), canvas, "R0",  -60.0f, 18.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R1",  -30.0f, 18.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R2",    0.0f, 18.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R3",   30.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R4",   60.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R5",   90.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R6",  130.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R7",  160.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R8",  200.0f, 16.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R9",  230.0f, 16.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R10", 260.0f, 16.0f));
    lines.push_back(makeLine(scene.get(), canvas, "R11", 290.0f, 16.0f));

    auto* behaviorGO = scene->createGameObject("TestBehavior");
    behaviorGO->setParent(canvas);
    behaviorGO->addComponent<ReflectionTestBehavior>(std::move(lines));

    return scene;
}
