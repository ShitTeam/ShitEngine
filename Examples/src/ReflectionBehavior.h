#pragma once
// ═══════════════════════════════════════════════════════════════
// ReflectionTestBehavior —— 运行时反射自检（原 ReflectionTestScene 内嵌行为）
// 通过 .scene 的 m_lineNames 字段配置输出行对象；onStart 执行自检并回写文本
// ═══════════════════════════════════════════════════════════════

#include <ShitEngine.h>

#include <sstream>
#include <string>
#include <vector>

#include "Behaviors.h"
#include "ReflectionTestTypes.h"

class SHIT_REFLECT(BlackList) ReflectionTestBehavior : public Shit::Behavior {
    SHIT_REFLECT_BODY(ReflectionTestBehavior)
public:
    ReflectionTestBehavior() = default;
    void setLineNames(const std::string& names) { m_lineNames = names; }

    void onStart() override {
        m_lines = example_helpers::resolveTextLines(example_helpers::ownerScene(this), m_lineNames);
        runReflectionTests();
        updateUIText();
    }

private:
    void runReflectionTests() {
        ST_CORE_INFO("========== Reflection System Tests ==========");

        // Test 1: Count
        size_t total = Shit::TypeRegistry::Count();
        ST_CORE_INFO("[Count] Registered types: {}", total);
        m_results.push_back(std::string("Total types: ") + std::to_string(total));

        // Test 2: Query by name
        const auto* tpInfo = Shit::TypeRegistry::Get("TestPlayer");
        if (tpInfo) {
            ST_CORE_INFO("[Get-by-name] Found '{}', size={}, fields={}",
                tpInfo->name, tpInfo->size, tpInfo->fields.size());
            m_results.push_back(std::string("Get(\"TestPlayer\"): ") +
                tpInfo->name + " (" + std::to_string(tpInfo->fields.size()) + " fields)");
        } else {
            m_results.push_back("Get(\"TestPlayer\"): FAILED");
        }

        // Test 3: Base type
        if (tpInfo && tpInfo->baseType) {
            ST_CORE_INFO("[BaseType] {} -> {}", tpInfo->name, tpInfo->baseType->name);
            m_results.push_back(std::string("Base: ") + tpInfo->baseType->name);
        }

        // Test 4: Fields
        if (tpInfo) {
            for (size_t i = 0; i < tpInfo->fields.size(); ++i) {
                const auto& f = tpInfo->fields[i];
                ST_CORE_INFO("  [Field {}] name={}, offset={}, size={}, type={}",
                    i, f.name, f.offset, f.size, f.typeName);
                std::stringstream ss;
                ss << "  Field: " << f.name << " (" << f.typeName << ", off=" << f.offset << ")";
                m_results.push_back(ss.str());
            }
        }

        // Test 5: Query by type_index
        if (const auto* tpByType = Shit::TypeRegistry::Get<TestPlayer>()) {
            ST_CORE_INFO("[Get<T>()] Found '{}' by type_index", tpByType->name);
            m_results.push_back(std::string("Get<TestPlayer>(): ") + tpByType->name);
        } else {
            m_results.push_back("Get<TestPlayer>(): FAILED");
        }

        // Test 6: Nonexistent type
        if (!Shit::TypeRegistry::Get("NonExistent")) {
            ST_CORE_INFO("[NotFound] Get(\"NonExistent\") correctly returned null");
            m_results.push_back("Get(\"NonExistent\"): null (OK)");
        }

        // Test 7: Builtin types
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

        // Test 8: Builtin via template
        if (const auto* intByType = Shit::TypeRegistry::Get<int>(); intByType && intByType == intInfo) {
            ST_CORE_INFO("[Get<int>()] matches Get(\"int\") — OK");
            m_results.push_back("Get<int>() == Get(\"int\"): OK");
        } else {
            m_results.push_back("Get<int>() == Get(\"int\"): FAILED");
        }

        // Test 9: ForEach
        Shit::TypeRegistry::ForEach([](const Shit::TypeInfo& info) {
            ST_CORE_INFO("  {}", info.name);
        });
        m_results.push_back(std::string("ForEach: ") + std::to_string(Shit::TypeRegistry::Count()) + " types listed");

        // Test 10: Prefab Capture → JSON round-trip → instantiate
        runPrefabTest();

        // Test 11: WeakComponentRef invalidation
        runWeakRefTest();

        ST_CORE_INFO("=============================================");
    }

    void runPrefabTest() {
        Shit::Scene* scene = getOwner() ? getOwner()->getScene() : nullptr;
        if (!scene) {
            m_results.push_back("Prefab test: no scene");
            return;
        }
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

        auto json = prefab.toJson();
        auto prefab2 = Shit::Prefab::FromJson(json);
        m_results.push_back(std::string("Prefab JSON round-trip: ") +
            std::to_string(prefab2.getComponents().size()) + " comps");

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
    }

    void runWeakRefTest() {
        Shit::Scene* scene = getOwner() ? getOwner()->getScene() : nullptr;
        if (!scene) {
            m_results.push_back("WeakRef test: no scene");
            return;
        }
        auto* weakGO = scene->createGameObject("WeakRefSource");
        weakGO->addComponent<Shit::TransformComponent>();
        auto ref = weakGO->getWeakRef<Shit::TransformComponent>();
        const bool before = ref.valid();

        weakGO->removeComponent<Shit::TransformComponent>();
        const bool after = ref.valid();

        if (before && !after) {
            ST_CORE_INFO("[WeakRef] 移除组件后弱引用失效 — OK");
            m_results.push_back("WeakRef invalidation: OK");
        } else {
            ST_CORE_ERROR("[WeakRef] 弱引用失效检测失败 (before={}, after={})", before, after);
            m_results.push_back("WeakRef invalidation: FAILED");
        }
    }

    void updateUIText() {
        for (size_t i = 0; i < m_lines.size(); ++i) {
            m_lines[i]->setText(i < m_results.size() ? m_results[i] : "");
        }
    }

    SHIT_META(({.displayName = "行对象名（R0,R1,...）", .tooltip = "解析多条 UIText 对象显示测试结果"}))
    std::string m_lineNames;
    // 容器不可序列化，必须显式 Disable（理由见 Behaviors.h：libclang 退化 "int" 注册会损坏内存）
    SHIT_META(Disable)
    std::vector<std::string> m_results;   ///< 运行时结果（容器类型不可序列化）
    SHIT_META(Disable)
    std::vector<Shit::UIText*> m_lines;   ///< 运行时按名解析（容器类型不可序列化）
};