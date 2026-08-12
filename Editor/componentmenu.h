#pragma once
// ═══════════════════════════════════════════════════════════════
// 添加组件菜单（Unity 风格）：场景树右键与检查器底部共用
// 从 TypeRegistry 收集所有 Component 派生的可实例化类型，构建菜单；
// 目标对象已持有的组件类型置灰（Unity "Add Component" 同款行为）。
// ═══════════════════════════════════════════════════════════════

#include <ShitEngine/GameObject/GameObject.h>
#include <ShitEngine/Reflection/TypeInfo.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <vector>

/// 判断反射类型是否为 Component 派生（沿基类链查找 "Component"）
inline bool isComponentDerived(const Shit::TypeInfo *ti)
{
    for (const Shit::TypeInfo *b = ti; b; b = b->baseType)
        if (b->name == "Component") return true;
    return false;
}

/// 收集全部可添加的组件类型（Component 派生 + 有反射工厂），按名称排序
inline std::vector<const Shit::TypeInfo *> collectAddableComponentTypes()
{
    std::vector<const Shit::TypeInfo *> comps;
    Shit::TypeRegistry::ForEach([&](const Shit::TypeInfo &ti) {
        if (isComponentDerived(&ti) && ti.factory)
            comps.push_back(&ti);
    });
    std::sort(comps.begin(), comps.end(),
              [](const Shit::TypeInfo *a, const Shit::TypeInfo *b) { return a->name < b->name; });
    return comps;
}

/// 构建"添加组件"菜单：已持有的类型置灰并标注「（已有）」，选择后回调 onPick
inline QMenu *buildAddComponentMenu(QWidget *parent, Shit::GameObject *target,
                                    std::function<void(const Shit::TypeInfo *)> onPick)
{
    auto *menu = new QMenu(QObject::tr("添加组件"), parent);

    for (const Shit::TypeInfo *ti : collectAddableComponentTypes()) {
        // 已有同类型组件时置灰（addComponentInstance 会丢弃重复实例，直接禁用更清晰）
        const bool has = target && target->getComponents().count(ti->typeIndex) > 0;
        auto *act = menu->addAction(QString::fromStdString(ti->name)
                                    + (has ? QStringLiteral("（已有）") : QString()));
        act->setEnabled(!has);
        if (!has)
            QObject::connect(act, &QAction::triggered, parent, [onPick, ti] { onPick(ti); });
    }
    return menu;
}