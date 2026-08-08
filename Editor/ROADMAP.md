# 编辑器开发路线图（ROADMAP）

> 本文档为 ShitEngine 编辑器（`Editor/`）与运行时场景管线的分阶段开发规划，使用中文。
> 规划日期：2026-08-06。状态：P6–P13 全部实现，编辑器路线图完成（剩余为打磨项）。

## 0. 核心架构决策

编辑器 + 运行时采用**场景数据驱动**引严格模式：

> **.scene 文件是场景的唯一来源** —— 编辑、运行、关卡切换共用同一格式与同一加载器；
> **用户 DLL（脚本插件）零场景搭建代码**，只提供可反射的组件/行为类型。

四条已确认的决策：

| # | 决策 | 影响 |
|---|------|------|
| 1 | 场景只能通过 `.scene` 文件定义，插件不再导出场景工厂函数 | 移除 ABI `CreateMainScene`，ABI 升级 **v2** |
| 2 | 插件入口简化到只注册反射类型（`RegisterPluginTypes`），可选 `OnPluginStart` 钩子留待需要时再加 | 插件 = 脚本库 |
| 3 | 运行时的关卡切换也走 `.scene`：新增 `SceneManager::LoadSceneFromFile(path)` | 首场景与切关同一机制 |
| 4 | 现有 ExamplePlugin 四个测试场景（PhysicsTest/UITest/InputMapping/ReflectionTest）全部迁移为 `.scene` 文件，删除搭建代码 | 行为/自定义组件保留在 DLL 中，反射后进 `.scene` |

可行性依据：

- 插件内自定义 `Behavior` / `Component` 只要标记 `SHIT_REFLECT`（含 `SHIT_REFLECT_BODY` 反射工厂），启动时经 `RegisterPluginTypes` 注册进 `TypeRegistry`；
- `.scene` 反序列化走 `Prefab::FromJson` → `TypeRegistry::Get(typeName)` → `ti->Create()` → `addComponentInstance`，与代码搭建完全等价；
- 编辑器「添加组件」菜单已通过反射枚举 `Component` 派生类（`Editor/scenetree.cpp:20-30`），自定义类型天然可用。

---

## 1. 现状盘点（P1–P5 已完成，2026-08）

### 已完成能力

| 能力 | 位置 |
|------|------|
| 四面板：双视口（场景/运行）+ 场景树 + 属性 + 日志 | `Editor/mainwindow.cpp:83-115` |
| 菜单：文件（新建/打开/保存/退出）+ 帮助 | `Editor/mainwindow.cpp:117-129` |
| 播放/停止工具栏（`Game::SetPaused`） | `Editor/mainwindow.cpp:235-253`, `preview.cpp:108-113` |
| 进程内引擎预览：独立 `EngineContext` + 隐藏窗口离屏 | `Editor/preview.cpp:44-48` |
| 60Hz 手动驱动帧 + 双 pass 离屏渲染（游戏相机驱动逻辑 / 编辑相机只重渲染） | `Editor/preview.cpp:128-165` |
| 视口拾取（正变换 AABB 命中） | `Editor/mainwindow.cpp:255-316` |
| Gizmo 平移手柄（沿屏幕轴的 X/Y 拖拽） | `Editor/viewport.cpp:108-134, 154-215` |
| 反射属性面板（float/int/bool/Vector2/string/枚举），每帧回读 | `Editor/inspector.cpp:86-243` |
| 场景树只读模型 + 右键新建对象/添加组件/删除 | `Editor/scenetree.cpp:78-98` |
| 场景 JSON 存取（v1：扁平 `{"objects":[{name,data}]}`） | `Editor/mainwindow.cpp:149-225` |

### 功能缺口（按影响排序）

1. **场景层级不序列化**：`saveScene` 输出扁平列表，父子关系保存/加载丢失
2. **纹理路径不落盘**：`SpriteRenderer::m_sprite` 标记 `readOnly`，存档中 fields 为空，贴图保存后丢失
3. **无编辑会话安全**：无 dirty 标记、无保存提示、打开失败无回滚（对象已删）、无最近场景
4. **无撤销/重做**
5. **无资源管线**：无项目目录概念、无资源面板、无拖拽创建对象
6. **播放态无输入转发**：运行视图不接收键盘/鼠标，游戏输入无法驱动
7. **日志面板未接引擎日志**：`LogWidget` 注释"P2 起可对接"（`logwidget.h:8-9`），实际未对接
8. **检查器残缺**：Color 无编辑器、未注册枚举只读展示、`size_t` 无控件、基类字段不遍历
9. **Gizmo 只有平移**：无旋转/缩放模式，轴方向硬编码为屏幕轴
10. **拾取仅 AABB**：无遮挡语义，仅支持 SpriteRenderer 对象
11. **场景树不可编辑**：无重命名/拖拽改层级/多选
12. **硬编码测试场景**：`preview.cpp:51-72` 每次启动重建 player + 双相机，纹理用临时棋盘格 BMP（167-204）

---

## 2. 技术债与贯穿性风险

| # | 风险 | 说明 |
|---|------|------|
| 1 | 反射 `.gen.h` 重新生成 | 改带 `SHIT_FIELD` 的头文件后必须重扫：BUILD_TOOLS=ON 自动，OFF 手动 `cmake --build . --target run-reflectionscanner`；否则编辑器/序列化看不到新字段 |
| 2 | 迭代中删除对象 | 编辑器下 `Scene::removeGameObject` 当场 erase（`Scene.cpp:149-180`），遍历中删会迭代器失效（cfa3b8d 教训）；保持「先收集再删」模式 |
| 3 | 写字段绕过 setter | 编辑器与 Prefab 反序列化都走 `FieldInfo::GetFieldPtr` 直写内存，不触发 setter；涉及引擎状态的字段（如精灵纹理）需 `onAfterDeserialize` 钩子 |
| 4 | EngineContext 切换 | 编辑器多个 context 并存，插件类型注册、面板操作前都必须 `setCurrent(preview)` |
| 5 | 隐藏窗口输入 | `Input::GetMousePosition` 从隐藏 SDL 窗口轮询，播放态转发输入前需引擎补 `SetMousePosition` |
| 6 | ABI 变更 | v1 → v2 一次性破坏旧插件（本次仅涉及 Runtime 内部契约，一次性接受） |
| 7 | 测试场景依赖 | 硬编码搭建 + 临时 BMP 是 P1 的权宜，P6 起逐步替换为 `.scene` 数据 |

---

## 3. 阶段路线图

### P6 场景数据驱动核心（引擎 + Runtime + 编辑器接入）—— ✓ 已实现（2026-08-06）

> 状态：引擎 + Runtime 代码完成并在 `out/build/x64-debug`（MinGW + Ninja）编译通过，
> Runtime 已实跑验证 .scene 加载（`Scenes/Preview.scene`：对象创建、图层序列化、`game_camera` 兜底、
> 纹理路径反序列化重载）。编辑器 `mainwindow` 已接入 `SceneSerializer`，待 Qt 环境（MSVC+Qt6）编译验证。

**目标**：`.scene` 成为唯一场景来源，全链路（编辑 ↔ 运行 ↔ 切关）走同一格式。

**引擎改动**
- 新增 `Scene/SceneSerializer.h/.cpp`：`toJson(scene)` / `fromJson(json, scene)`，详见 §4
- `SpriteRenderer` 新增反射字段 `texturePath` + `Component::onAfterDeserialize()` 钩子（详见 §4）
- `SceneManager::LoadSceneFromFile(path)`

**Runtime 改动**
- `config.json` 新增顶层 `"scene"` 字段
- `PluginManager` 移除 `CreateMainScene`（ABI v2），`main.cpp` 流程改为：加载插件 → 注册类型 → 读 `.scene` → LoadScene

**编辑器改动**
- `saveScene`/`openScene` 改用共享 `SceneSerializer`，删除手写 JSON 拼装/解析
- 新建场景模板自动含 `game_camera`（编辑器相机 `scene_camera` 为约定名，不落盘）

**验收**：见 §5.5。

---

### P7 · 插件共享加载 + 示例迁移 —— ✓ 已实现（2026-08-06）

> 状态：`PluginManager` 已移入引擎（`Engine/include/ShitEngine/Plugin/PluginManager.h`），Runtime 与编辑器共用；
> 编辑器启动时从 exe 同目录 `config.json` 加载插件。四个测试场景（PhysicsTest / UITest / InputMapping /
> ReflectionTest）已迁移为 `Examples/scenes/*.scene`（行为类重构为 `SHIT_REFLECT` 脚本库，指针引用改对象名
> 解析，UIButton onClick 改 `ButtonClickDemo` 行为，物理重力改 `GravityConfig` 行为），搭建代码与
> `CreateMainScene`/SceneDumper 已删除。Runtime 默认配置 `PhysicsTest.scene` 验证通过。
> 额外修复：插件反射字段类型带 `Shit::` 前缀导致序列化/检查器不识别（Prefab/Inspector 归一化）；
> `AudioTrack` 补 `SHIT_API` 导出（引擎外按值持有 EngineContext 时的链接问题）。

**目标**：编辑器能编辑含自定义行为的场景；示例全部改为 `.scene`。

**改动**
- 引擎：把 `PluginManager` 从 `Runtime/src` 移入 `Engine/src/ShitEngine/Plugin/`（Editor 与 Runtime 共用；编辑器加载插件后自定义类型才可实例化/序列化。类型注册沿用现有反射机制，无需引擎新组件）
- 示例：为 PhysicsTest / UITest / InputMapping / ReflectionTest 逐一写**一次性转换器**（搭建代码导出 `.scene` 入库 `Examples/scenes/`），行为类改为 `SHIT_REFLECT`；迁移完成后删除搭建代码
- 编辑器：加载插件 → 反射类型注册 → 添加组件菜单 / 检查器 / 序列化全链路支持

**验收**
- Runtime 直接配置 `PhysicsTest.scene` 跑出物理效果
- 编辑器打开含自定义行为的 `.scene`，可增删/编辑/保存；Runtime 复跑结果一致

---

### P8 · 编辑会话安全 —— ✓ 已实现（2026-08-06）

> 状态：已实现，并在本机 Qt 6.8.3 MSVC2022（BuildTools）环境编译通过。本次是编辑器改动首次在
> MSVC 下全量编译验证（P6/P7 编辑器改动一并覆盖，修复 lambda 隐式捕获 `this`、`QPushButton`
> 完整类型两处 MSVC 编译错误）。行为未做手测运行，待用户在 Qt Creator 运行验收。

**目标**：不丢数据。（原 P6 编辑器部分独立阶段化）

**改动**（纯编辑器）
- dirty 标记：Gizmo 拖拽结束（`Viewport::gizmoDragFinished`）、检查器字段变更（`Inspector::fieldEdited`）、树操作（`SceneTree::sceneEdited`）三路信号 → `MainWindow::setDirty`；标题栏 `<场景名> * - ShitEngine 编辑器`
- 关闭 / 新建 / 打开前未保存提示（保存 / 不保存 / 取消；保存失败或取消则中止当前操作）
- 打开失败回滚：打开前全量 `SceneSerializer::toJson` 快照（含编辑器相机），`fromJson` 异常时清场并从快照整体恢复；JSON 解析失败发生在触碰场景前，无需回滚
- 最近场景：`QSettings`（ShitTeam/ShitEngineEditor）存最近 5 条，「文件 → 最近场景」子菜单一键打开（自动剔除已删除文件）
- 附赠：`Ctrl+N/O/S`、`Ctrl+Shift+S` 快捷键 + 「场景另存为…」

**验收**：任意编辑后关窗有提示；打开损坏/不存在文件原场景不丢；最近场景菜单可用。

---

### P9 · 撤销/重做 —— ✓ 已实现（2026-08-06）

> 状态：已实现并在本机 Qt 6.8.3 MSVC2022（BuildTools）环境编译通过（EXITCODE=0）。
> 行为未手测运行，待用户在 Qt Creator 运行验收。

**目标**：编辑行为可逆。

**改动**（纯编辑器）
- 新增 `undostack.*`：场景级快照命令栈（begin/commit 事务；before/after 全场景 `SceneSerializer` JSON 对比，排除编辑器相机；无差异不入栈）
- 三源接线：Gizmo 拖拽（press→begin / release→commit）、检查器（首次 valueChanged begin，`editingFinished` commit；按钮/下拉即时 commit）、树操作（操作前 begin / 操作后 commit）
- `编辑` 菜单 + `Ctrl+Z` / `Ctrl+Shift+Z`；运行态（播放）不记录且禁用
- `UndoStack` 双栈（undo/redo），新编辑清空 redo；撤销/重做后场景重建并联动树/检查器/Gizmo
- dirty 改进：转发到「最后保存快照」对比——撤回到已保存状态时标题 `*` 自动消失

**验收**：平移、改属性、删对象、新建均可逐级撤销/重做，树与检查器联动刷新。

---

### P10 · 资产浏览与创建管线 —— ✓ 已实现（2026-08-06）

> 状态：已实现并编译通过（Qt 6.8.3 MSVC）。待用户运行验收。

**目标**：素材进场景的门路。

**改动**（纯编辑器，`SpriteRenderer::texturePath` 已在 P6 就位）
- 新增 `assetsdock.*`：资源面板（左侧，场景树上方）——目录路径可编辑/浏览、`QSettings` 持久化（"projectDir"）
- 过滤代理：目录全显，文件只留 png/jpg/jpeg/bmp/wav/ttf/otf/scene（隐藏 . 开头文件/目录）
- 拖拽图片到场景视口（dragEnter/Move/Drop 接受 text/uri-list 图片）→ 创建 `GameObject(Transform+SpriteRenderer+texturePath)`，落在光标世界坐标；自动选中 + 撤销记录
- 双击资源面板 `.scene` → 走原本的带提示打开路径

**验收**：素材目录可浏览；拖图到视口生成显示精灵；双击场景文件打开。

---

### P11 · 编辑操作增强 —— ✓ 已实现（2026-08-06）

> 状态：已实现并在 Qt 6.8.3 MSVC2022（BuildTools）环境编译通过（EXITCODE=0，含引擎改动重建）。
> 行为未手测，待用户运行验收。

**目标**：场景树与场景操作手感对齐 Unity/Godot。

- 场景树：双击/F2 重命名（`setData`/EditRole + `dataEdited` 信号）、拖拽改层级（InternalMove + 自定义 mime 行号路径 → `setParent`，防环检查）、尚未做多选删除
- Gizmo：三模式（移动/旋转/缩放，工具栏 + `Q/W/E` 快捷键）；旋转 15° 量子化（Ctrl 5°）；移动 `Ctrl` 10px 网格吸附；缩放 `Ctrl` 0.1 吸附
- 拾取升级：精灵命中按 zIndex 取最上；无精灵命中时「变换点」拾取（相机/空对象等可选中，编辑器相机除外）

**验收**：树内拖拽重排层级、双击改名生效；Gizmo 三模式与吸附手感正确；拾取能选中无精灵的对象（相机/空对象）。

---

### P12 · 播放器体验 —— ✓ 已实现（2026-08-06）

> 状态：已实现并编译通过（EXITCODE=0）。行为未手测，待用户运行验收。

**目标**：在编辑器内真正「玩」游戏并看到引擎日志。

- 播放态输入转发：运行视口捕获 Qt 键鼠事件（`eventFilter`）→ 合成 `SDL_Event` → `Input::HandleEvent`；鼠标坐标经新引擎 API `Input::SetMousePosition` 注入（隐藏窗口下轮询失效）；运行视口需先点击获得焦点
- 引擎 `Log::SetMessageCallback`：自定义 spdlog sink 把 `ST_CORE_*` / `ST_*` 日志转发到日志面板（`[引擎]/[游戏]` 前缀 + 等级着色，跨线程 QueuedConnection）
- 现状：按键映射覆盖字母/数字/功能键/方向/常用符号；Ctrl/Alt/Shift 以左键扫描码表示

**验收**：播放态下 WASD/鼠标驱动测试场景；引擎 `ST_CORE_*` 日志滚动进入日志面板。

---

### P13 · 产品化打磨 —— ✓ 已实现（2026-08-06）

> 状态：已实现并编译通过（EXITCODE=0）。行为未手测，待用户运行验收。

- 窗口图标（程序化生成，免资源管理）、`mainwindow.ui` 模板残留清理（windowTitle 模板名移除）
- 全套快捷键补全：`Del` 删除对象（树焦点时，重命名编辑中豁免）；Ctrl 组 / QW E / F2 已在 P8–P11 就位
- Dock 布局持久化：退出自动 `saveState`/`saveGeometry`，首启存出厂默认，「视图 → 恢复默认布局」重置
- 工具栏增补：撤销 / 重做（与菜单共享 QAction）
- 关于对话框更新：中英版本说明 + 完整快捷键表 + 架构一句话
- 「中英 UI」说明：当前界面文案统一中文（`tr()` 已留翻译位），完整双语切换留待后续

**验收**：首启后无模板残留；布局拖动后重启可恢复；所有主操作有快捷键。

---

## 4. 依赖关系

```
P6（场景 + 序列化地基） ──┬──> P7（插件共享 + 示例迁移）──> P12（播放器）
                          └──> P8（会话安全）──> P9（撤销）──> P11（编辑增强）
                          └──> P10（资产管线，依赖 P6 texturePath）
P12（播放器）可并行于 P8–P11
```

---

## 5. P6 详细规格 —— 场景数据驱动核心

### 5.1 `.scene` v2 格式设计

```json
{
  "version": 2,
  "objects": [
    { "name": "Play_Area",  "parent": -1, "data": [ { "type": "TransformComponent", "fields": { "m_position": [0,0], ... } } ] },
    { "name": "Enemy",       "parent": 0,  "data": [ { "type": "TransformComponent", "fields": {...} }, ... ] }
  ]
}
```

- `parent`：父对象在 `objects` 数组中的下标，`-1` = 根对象；**保存时后序遍历**保证父先于子出现
- 加载顺序：`createGameObject(name)` → `Prefab::FromJson(data).instantiate(scene, name)` → `setParent`（按 parent 下标还原）
- 兼容 v1：文件无 `version` 字段，或对象无 `parent` 字段 → 全部视为根对象（现有平铺格式可读）
- 相机兜底：`fromJson` 结束后场景内无已启用 `CameraComponent` → 自动补 `game_camera`（Transform + CameraComponent 缩放 1.0）

### 5.2 引擎改动清单

**新增 `Scene/SceneSerializer.h` + `.cpp`**

```cpp
namespace Shit {

/// @brief 全场景（对象 + 层级 + 组件）与 JSON 双向转换（.scene 事实标准格式）
class SHIT_API SceneSerializer {
public:
    /// 序列化整个场景（包含所有对象与层级，v2 格式）
    static nlohmann::json toJson(Scene* scene);

    /// 把 JSON 追加实例化进目标场景（调用方负责前置清场；
    /// 相机兜底由本函数统一处理）。异常时可能部分创建，
    /// 调用方应先 toJson 快照旧场景用于回滚。
    static void fromJson(const nlohmann::json& doc, Scene* scene);
};

} // namespace Shit
```

**文件 `Scene/SceneManager.h` + `.cpp`**

```cpp
/// @brief 从 .scene 文件加载并替换当前场景（切关/启动共用）
/// @return 成功返回 true；文件不存在/解析失败/无对象返回 false，保持原场景不变
bool SHITAPI LoadSceneFromFile(const std::string& path);
```

**文件 `Component/Component.h`（基类）**

```cpp
/// @brief 反序列化完成后回调（Prefab::apply 每个组件调用一次，编辑器/Rt 复用）
virtual void onAfterDeserialize();
```

**文件 `Render/SpriteRenderer.h` + `.cpp`（纹理路径持久化）**

- 新增 `std::string m_texturePath`（`SHIT_FIELD` + `SHIT_META(({.displayName="纹理路径"}))`）
- `setTexturePath(path)`：同步写 `m_texturePath` + 触发原有纹理加载逻辑（`SpriteRenderer::setTexturePath` 已存在）
- `getTexturePath()` / `override onAfterDeserialize()`：路径非空 → 重新 `ResourceManager::LoadTexture(path)` 重建 `m_sprite`

**文件 `GameObject/Prefab.cpp`（`apply` 末尾）**

```cpp
// 现有循环：动 addComponentInstance 后
for (auto& comp : components) if (comp) comp->onAfterDeserialize();
```

**注意**：改 `SpriteRenderer.h`/`Component.h` 后必须重新生成 `.gen.h`（BUILD_TOOLS=OFF 手动 `cmake --build . --target run-reflectionscanner`），再重编 `ShitEngine` 与 Editor。

### 5.3 Runtime 改动清单

| 文件 | 改动 |
|------|------|
| `Runtime/config.json` | 新增顶层 `"scene": "Scenes/<name>.scene"`（相对 exe 同目录） |
| `Runtime/src/PluginManager.h` | 删除 `CreateSceneFn`/`createScene`/`CreateAllScenes`；ABI 常量改为 2 |
| `Runtime/src/PluginManager.cpp` | `loadPlugin` 不再要求 createSc（缺 `RegisterTypes` 与其它导出仍要求）；删除 `CreateAllScenes` |
| `Runtime/src/main.cpp` | 第 4 步：读 config 的 `scene` 字段 → `LoadSceneFromFile`；无该字段 → 创建空场景+默认相机（兜底统一走 SceneSerializer） |

### 5.4 编辑器改动清单

| 文件 | 改动 |
|------|------|
| `Editor/mainwindow.cpp` | `saveScene`（202-225）：改用 `SceneSerializer::toJson`；`openScene`（149-200）：先全量快照 → 收集删除 → `fromJson` 追加；删除手写 JSON 拼装与手动相机兜底（179-186） |
| `Editor/mainwindow.cpp` | `newScene` 保留 `scene_camera` + `game_camera`（游戏相机作为模板保留） |
| `Editor/inspector.cpp` | SpriteRenderer 现在走 string & 字段：自动获得「纹理路径」编辑框，无需改动 |

### 5.5 验收清单

1. 层级往返：建父对象 + 子对象 → 保存 → 新建 → 打开 → 层级还原、名称保留
2. 贴图往返：为精灵设纹理 → 保存 → 打开 → 纹理重现（检查器在「纹理路径」文本框正确显示路径值，加载后贴图自动重载）
3. 运行一致：Editor 保存的 `.scene` 直接配置到 Runtime 的 `config.json` → 运行结果与编辑器一致
4. 切关：游戏内调用 `SceneManager::LoadSceneFromFile("level2.scene")` 生效（新场景替换旧场景）
5. 空启动：无 `"scene"` 配置 → 启动空场景 + 默认相机，不崩溃
6. 兼容：旧的 v1 `.scene` 文件（对象无 `parent` 字段）可正常打开
7. 容错：文件不存在 / JSON 损坏 → `LoadSceneFromFile` 返回 false，原场景不变

---

## 6. 文档维护约定

- 每个阶段完成后在本文件对应小节打勾 ✓，并同步更新 `CHANGELOG.md`（Keep a Changelog）+ `README.md` 的编辑器状态
- 与 `AGENTS.md`（含 `CLAUDE.md` 同源）保持同步：新增编辑器文件说明请更新「编辑器（Editor）」章节