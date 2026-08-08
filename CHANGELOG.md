# Changelog

本项目的所有重要变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 新增

- **产品化打磨（P13，纯编辑器）**：
  - 窗口图标（程序化生成）、`mainwindow.ui` 模板残留清理
  - `Del` 快捷键删除对象（树焦点生效、重命名编辑中豁免）；工具栏增补撤销/重做
  - Dock 布局持久化：退出自动记忆（`saveState`/`saveGeometry`），「视图 → 恢复默认布局」
  - 关于对话框更新（版本信息 + 全套快捷键表）
- **编辑操作增强（P11，引擎+编辑器）**：
  - 场景树：双击/F2 重命名、拖拽改层级（InternalMove，防环；`setParent` 即时生效并记入撤销）
  - Gizmo 三模式：移动/旋转/缩放（工具栏 + `Q/W/E` 快捷键）；旋转 15° 量子化（Ctrl 5°）、移动 Ctrl 10px 网格吸附、缩放 Ctrl 0.1 吸附
  - 拾取升级：精灵按 zIndex 取最上优先；无精灵时「变换点」拾取支持相机/空对象（编辑器相机除外）
- **播放器体验（P12，引擎+编辑器）**：
  - 引擎 `Input::SetMousePosition` — 编辑器播放态注入鼠标逻辑坐标（隐藏窗口下 SDL 轮询失效）
  - 引擎 `Log::SetMessageCallback` — spdlog 自定义 sink 转发 `ST_CORE_*`/`ST_*` 日志
  - 播放态运行视口 Qt 键鼠事件合成 `SDL_Event` → `Input::HandleEvent`（WASD/鼠标/滚轮可驱动游戏）
  - 日志面板实时滚动引擎日志（`[引擎]/[游戏]` 前缀 + warn/error 着色，跨线程 QueuedConnection）
- **撤销/重做（P9，纯编辑器）** — 场景级快照命令栈 `UndoStack`（`Editor/undostack.*`）：
  - 编辑手势事务化：Gizmo 拖拽（press→begin / release→commit）、检查器字段（首次变更 begin，`editingFinished`/按钮/下拉 commit）、场景树操作（操作前/后）→ before/after 全场景 `SceneSerializer` JSON 对比，无差异不入栈
  - 「编辑 → 撤销/重做」+ `Ctrl+Z` / `Ctrl+Shift+Z`；运行态（播放）不记录且禁用
  - 撤销/重做后整体重建场景并联动场景树/检查器/Gizmo；dirty 与「最后保存快照」对比——撤回到保存态时标题栏 `*` 自动消失
- **编辑器布局与默认场景（P9/P10 收尾）**：
  - 删除默认测试场景：启动即空场景（仅 `game_camera` + `scene_camera` 双相机），移除临时棋盘格贴图与 `PreviewMover` 行为
  - 场景树隐藏 `scene_camera`（编辑器相机不列入层级）
  - Dock 全独立：场景视口 / 运行视口 / 检查器均为独立 `QDockWidget`（可拖动、停靠、浮动，对齐 Unity 面板习惯），中央改空占位；资源面板移至底部与日志并排
- **资源浏览与创建管线（P10，纯编辑器）** — 新增 `Editor/assetsdock.*` 资源面板：
  - 目录树过滤 png/jpg/jpeg/bmp/wav/ttf/otf/scene，路径 `QSettings` 持久化，可编辑/浏览切换
  - 拖拽图片到场景视口 → 在光标世界坐标创建 `GameObject(Transform + SpriteRenderer(texturePath))` 精灵，自动选中并记入撤销
  - 双击资源面板 `.scene` → 带未保存提示打开
- **编辑会话安全（P8，纯编辑器）** — 不丢数据：
  - dirty 标记：Gizmo 拖拽结束 / 检查器字段变更 / 场景树操作 → 置脏，标题栏 `<场景名> *`
  - 关闭 / 新建 / 打开前未保存提示（保存 / 不保存 / 取消，保存失败或取消则中止）
  - 打开失败回滚：打开前全量 `SceneSerializer::toJson` 快照，`fromJson` 异常时清场并从快照整体恢复原场景
  - 最近场景：「文件 → 最近场景」子菜单（`QSettings` 持久化 5 条，自动剔除已删除文件）
  - 快捷键 `Ctrl+N/O/S`、`Ctrl+Shift+S` 与「场景另存为…」
- **场景数据驱动（P6）** — `.scene` 文件成为场景唯一来源（编辑 / 运行 / 切关共用）：
  - `SceneSerializer`（`Scene/SceneSerializer.h`）全场景序列化：对象 + 层级（v2 格式，`parent` 下标引用，兼容 v1 平铺）+ 组件（复用 Prefab）；加载后无相机自动补 `game_camera`
  - `SceneManager::LoadSceneFromFile(path)` — 从 .scene 文件加载 / 切换场景（启动、关卡切换统一入口）
  - `Component::onAfterDeserialize()` 反序列化钩子 — `Prefab::apply` 逐组件调用，用于重建「反射直写字段绕过的 setter 状态」
  - `SpriteRenderer` 新增反射字段 `texturePath` — 精灵纹理路径随 .scene 持久化，反序列化后自动重载（此前 `m_sprite` 只读、贴图丢失）
  - 编辑器存储/打开改用共享 `SceneSerializer`（替换手写 JSON），新增演示 `Runtime/Scenes/Preview.scene`

### 变更

- **插件 ABI v2** — 移除 `CreateMainScene` 场景工厂导出，插件 = 脚本库（只导出身份 + `RegisterPluginTypes`），插件不再写场景搭建代码
- **Runtime 启动流程** — 读 `config.json` 顶层 `"scene"` 字段 → `SceneManager::LoadSceneFromFile`；缺省启动空场景、由序列化器兜底默认相机

### 新增

- **PluginManager 移入引擎共享（P7）** — `Engine/Plugin/PluginManager.h`，Runtime 与编辑器共用；编辑器启动时从 exe 同目录 `config.json` 加载插件，自定义行为类型可实例化 / 编辑 / 序列化
- **示例迁移为数据驱动（P7）** — PhysicsTest / UITest / InputMapping / ReflectionTest 由 C++ 搭建改为 `Examples/scenes/*.scene`：
  - 行为重构为 `SHIT_REFLECT` 脚本库（`Behaviors.h` / `ReflectionBehavior.h`）：指针引用改「对象名」字段在 onStart 解析；UIButton onClick 改 `ButtonClickDemo` 行为；物理重力改 `GravityConfig` 行为
  - 删除搭建代码与 `CreateMainScene`，Runtime 默认场景 `PhysicsTest.scene`
- **反射类型名归一化** — 插件全局类字段（`Shit::Vector2` 等）在 Prefab 序列化 / 检查器中按裸类型名分派（此前带命名空间前缀导致字段被跳过）
- **AudioTrack 补 SHIT_API** — 引擎外按值持有 EngineContext 时析构符号跨 DLL 可链接

## [1.3.0] - 2026-08-02

> 🎉 **首个正式 Release**。v1.1 / v1.2 为开发阶段里程碑，本次随 v1.3.0 一起发布。

### 新增

- **编译期反射系统** — libClang 解析 AST + 代码生成：
  - `SHIT_REFLECT` / `SHIT_META` / `SHIT_ENUM` 标记宏，`SHIT_REFLECT_BODY` 内嵌 friend 声明
  - `TypeRegistry` 运行时查询：类型名、字段偏移/大小、编辑器显示名、数值范围
  - WhiteList / BlackList 两种反射模式，支持多 `SHIT_META` 元数据
  - 成员指针路径（`memberOffset(&T::field)`）在运行时计算偏移，跨 ABI 自校验
- **UI 组件反射** — UITransform / UICanvas / UIImage / UIText / UIButton / UITextInput 全部接入反射元数据，为编辑器属性面板铺路
- **Prefab 数据驱动** — 反射克隆 + JSON 序列化，`Prefab::Build` 统一数据驱动接口
- **WeakComponentRef** — 组件弱引用，`GameObject` 销毁后引用自动悬空，杜绝 UAF
- **ST_CORE_ASSERT** — 错误处理契约第三层（assert 宏）
- **SDK 优先的消费模型** — 预编译 SDK + `find_package(ShitEngine)` 一行接入，移除 FetchContent 方式（README / 文档站同步更新）

### 重构

- **EngineContext 容器** — 抽离全部 12 个单例进 `EngineContext`，支持多实例（编辑器进程内预览 / 单元测试）
- **组件-系统解耦** — Scene 统一广播 `registerComponent` / `unregisterComponent`，System 通过动态识别接管组件
- **移除场景栈** — 改为 `LoadScene` 单一当前场景 + 全局暂停（对齐 Unity/Godot），`LoadScene` 延迟到更新结束后应用，避免 UAF
- **RenderSystem 收敛** — 渲染层统一走 `Renderer` 抽象 + 墓碑化遍历
- **Input 合并** — 键盘/鼠标三态 + 动作/轴映射统一为类 Unity/Godot API
- **物理像素比例** — `LengthUnitsPerMeter` 改为实例字段，`RigidBody2D::onAttach` 幂等创建碰撞形状

### 修复

- 全面代码审查确认的 **43 个 Bug**（两轮审查，Workflow 验证）
- MSVC CI 构建失败 — 移除生成代码中的 ABI 相关 `static_assert`
- `SDL_Quit` 顺序、`TTF_Init` 布尔返回值判断、`Renderer` 静态方法空指针守卫
- `Game::Destroy` 空指针、`addComponent` 生命周期悬挂、场景切换悬垂
- 音频、事件、动画、UI 输入法、物理引擎等子系统残留问题
- 反射生成文件名命名空间化（`Shit__X.gen.h`），防跨命名空间冲突

## [1.2.0] - 开发里程碑

### 新增

- **UI 系统** — UITransform（锚点/枢轴布局）、UIImage、UIText、UIButton、UITextInput / UITextBox / UITextArea（含 IME 中文输入），UIRenderSystem 独立渲染管线
- **物理系统** — Box2D 3.1.1 封装：RigidBody2D / BoxCollider2D / CircleCollider2D，System priority=50，默认 32 像素/米
- **DLL 插件架构** — Runtime 动态加载插件 DLL，插件自带反射扫描与类型注册
- **FontManager** — 字体统一缓存管理

### 重构

- UI 子系统改用 ShitEngine 封装层（Renderer / KeyCode / Time / Color）
- pch.h 统一预编译头

## [1.1.0] - 开发里程碑

### 新增

- **核心架构** — Game / Scene / Component / System / GameObject 组件化架构，Behavior 脚本生命周期（onStart/onUpdate）
- **SDL3 渲染管线** — 逻辑分辨率 letterbox、最近邻缩放、像素对齐、多相机分屏、zIndex 排序
- **输入系统** — 键盘/鼠标 Down / Pressed / Released 三态检测
- **音频系统** — AudioPlayer 分层增益（master × group × track）、自动回收
- **配置系统** — `settings.json` JSON 配置，缺失时安全默认回退
- **资源管理** — 纹理/音频/字体懒加载与 RAII 回收，SpriteSheet 网格切割 + AnimationComponent 逐帧动画
- **事件总线** — 缓冲队列 EventBus，回调内可安全订阅/派发
- **结构化日志** — spdlog 多级日志，引擎/用户日志分离
- **Time** — DeltaTime / TotalTime / 帧率限制（SDL_AddTimer）
- **CI/CD** — GitHub Actions 四平台矩阵构建（Windows MinGW/MSVC、Linux、macOS）+ SDK 产物自动打包
