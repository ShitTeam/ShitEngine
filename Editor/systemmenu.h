#pragma once
// ═══════════════════════════════════════════════════════════════
// 添加系统菜单：镜像 componentmenu.h，收集 TypeRegistry 中 System 派生
// 的可实例化类型，构建菜单；已注册的置灰。
// ═══════════════════════════════════════════════════════════════

#include <ShitEngine/Scene/Scene.h>
#include <ShitEngine/Reflection/TypeInfo.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <vector>

/// 判断反射类型是否为 System 派生（沿基类链查找 "System"）
inline bool isSystemDerived(const Shit::TypeInfo *ti)
{
    for (const Shit::TypeInfo *b = ti; b; b = b->baseType)
        if (b->name == "System") return true;
    return false;
}

/// 收集全部可添加的系统类型（System 派生 + 有反射工厂），按名称排序
inline std::vector<const Shit::TypeInfo *> collectAddableSystemTypes()
{
    std::vector<const Shit::TypeInfo *> sysTypes;
    Shit::TypeRegistry::ForEach([&](const Shit::TypeInfo &ti) {
        if (isSystemDerived(&ti) && ti.factory)
            sysTypes.push_back(&ti);
    });
    std::sort(sysTypes.begin(), sysTypes.end(),
              [](const Shit::TypeInfo *a, const Shit::TypeInfo *b) { return a->name < b->name; });
    return sysTypes;
}

/// 构建"添加系统"菜单：已注册的置灰并标注「（已有）」，选择后回调 onPick
inline QMenu *buildAddSystemMenu(QWidget *parent, Shit::Scene *scene,
                                 std::function<void(const Shit::TypeInfo *)> onPick)
{
    auto *menu = new QMenu(QObject::tr("添加系统"), parent);

    for (const Shit::TypeInfo *ti : collectAddableSystemTypes()) {
        const bool has = scene && scene->hasSystem(ti->name);
        auto *act = menu->addAction(QString::fromStdString(ti->name)
                                    + (has ? QStringLiteral("（已有）") : QString()));
        act->setEnabled(!has);
        if (!has)
            QObject::connect(act, &QAction::triggered, parent, [onPick, ti] { onPick(ti); });
    }
    return menu;
}