# Changelog

本项目的所有重要变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 新增

- **组件 UUID + 组件引用字段（P20，引擎 + 扫描器 + 编辑器 + 示例）**：Unity 式"组件拖拽引用"基础设施——
  - **组件持久 UUID**：`Component` 基类新增 64 位随机 ID（`getUuid()/setUuid()`，`GenerateComponentUuid()`，0 保留为空引用），构造即分配；随 `.scene` 落盘（`Prefab::ComponentData.uuid`）——ID 跨编辑会话稳定；旧 `.scene` 无 `uuid` 字段时加载现场分配，向后兼容
  - **`ComponentRef<T>` 引用字段**（`GameObject/ComponentRef.h`）：字段内只存目标组件 UUID（8 字节），`get()/operator->/operator bool` 经当前场景索引懒解析；目标组件被移除/对象销毁后自动返回 nullptr——**永不悬垂**（与 `WeakComponentRef` 会话期弱引用互补：它是可序列化的持久引用）。运行时实例化（`Prefab::instantiate`）不恢复记录 ID，防复制实例共享 uuid 引用串线
  - **Scene UUID 索引**：`Scene::componentByUuid()` 公开查询 + 索引随组件生命周期维护（`registerComponent/unregisterComponent`、`GameObject` 挂载/卸下/清理全挂钩，覆盖不主动注册系统的组件如 TransformComponent）；随机 uuid 撞车自动重发并告警
  - **反射与序列化支持**：扫描器识别 `ComponentRef<具体组件类型>` 字段（剥 `Shit::` 前缀解析模板参数），生成 `.Ref("<目标类型>")` 链式标记；`FieldInfo::refType` 标识引用字段（`isReference()`）；Prefab 序列化按 8 字节 UUID 直读直写（0 = 空引用）
  - **检查器拖拽引用（编辑器）**：组件标题栏可拖拽（`ComponentHeaderLabel`，携带 UUID/类型名）；引用字段渲染为引用控件 `RefFieldWidget`（显示「对象名 (组件类型)」+「✕」清除按钮），接受组件头拖拽与**场景树对象拖入**（自动挑选第一个类型可赋值的组件；类型校验沿反射基类链做与运行期 dynamic_cast 同语义的判定）；编辑走既有 dirty 标记 + 撤销快照
  - 示例 `CoinDemo.h` 的计数文本改由 `ComponentRef<UIText>` 引用（检查器拖拽/序列化恢复，不再按名现查），`CoinCollect.scene` 各组件带 uuid 落盘验证全链路

### 修复

- **示例 Player 控制器反射化（示例）**：`Examples/src/Player.h` 是 P6 迁移前的遗留类，未加 `SHIT_REFLECT` 宏——扫描器不登记，检查器加不了、`.scene` 序列化不了（组件在编辑器里"不存在"）。补齐反射标记与 `speed` 字段元数据，Player 成为可编辑/可序列化的标准行为组件
- **项目设置保存丢失启动场景（P15 遗留，编辑器）**：设置对话框「启动场景」下拉只列 `<项目>/Scenes/` 目录下的 `.scene`——场景文件不在该目录（如放项目根目录）时下拉无匹配项，保存设置即 `setScenePath("")` **静默清除** config.json 的 `scene` 字段，下次启动项目退回空场景。修复：下拉追加「（当前）场景名」兜底项并默认选中——当前启动场景总能被保存
- **刚体暂缺 Transform 告警降噪（引擎）**：`.scene` 组件顺序不定导致的"缺少 TransformComponent"是自愈补建的正常瞬态，由 WARN 降为 DEBUG——每次场景加载不再刷 6 条告警（永久缺 Transform 的异常对象由补建循环静默跳过，不刷屏）
- **数据驱动场景物理系统丢失（P7 回归，引擎）**：P7 把示例从 C++ 搭建迁移为 `.scene` 数据驱动后，`PhysicsSystem2D` 从未被注册进场景（旧 `PhysicsTestScene.cpp` 的 `registerSystem<Shit::PhysicsSystem2D>()` 随搭建代码删除；`Scene::init` 只注册 Behavior/Render/UI 三系统）——Runtime 与编辑器的所有 `.scene` 场景**静默无物理**（刚体不落、重力无效）。修复（三层自愈，与引擎"补挂/补扫"机制一致）：
  - `RigidBody2D::onAttach` 未被任何系统认领时自动 `registerSystem<PhysicsSystem2D>()` 并重试认领——首个刚体挂载即拉起物理世界（Runtime / 编辑器实时加组件 / 撤销重做重建场景统一生效）
  - `onComponentAttached` 先注册进 `m_bodies` 再建体；`PhysicsSystem2D::update` 每帧补建"已注册但未建体"的刚体——`.scene` 组件顺序不定（Transform 在刚体之后反序列化）时不再永久丢失
  - 物理 API（`applyForce/applyImpulse/setLinearVelocity/setTransform/setBodyType`）调用前 `ensureBody()` 自愈建体——`onStart`（BehaviorSystem 先于物理系统执行）里调物理 API 不再静默落空
- **编辑器播放期悬垂指针崩溃（引擎 + 编辑器）**：播放中游戏逻辑销毁/新建对象（行为 `onUpdate` 里 `removeGameObject`、子弹回收、`LoadScene` 切关等）后，场景树行、视口 Gizmo、检查器仍持有已释放的 `GameObject*/Component*`，下一帧渲染/回读即 use-after-free 崩溃。修复：
  - 场景结构引入**代数（generation）**：任何对象增删/组件增删/改名/改父递增（`Scene::getGeneration/bumpGeneration`，`GameObject` 为友元）；新增 `Scene::containsGameObject` 供编辑器校验旧选中指针
  - 编辑器每帧 `onSceneFrameReady` 同步：场景指针或代数变化 → 重建场景树（不自动选中）→ 选中对象仍存活则恢复选中并重绑检查器/Gizmo，已被销毁则清空选中态；`SceneTree::setScene` 增加 `autoSelect` 参数、新增 `selectedObject()`
  - `EnginePreview::tick` 每帧跟随 `SceneManager::GetCurrentScene()`（播放切关后不再持有被销毁的旧场景）；相机被删后自愈补齐（`ensureCameras`），不再因 tick 提前 return 而冻结编辑器
  - `Viewport::drawGizmo` 二次防呆：选中对象不在场景则清除选中
  - 新增 `Game::SetIsRunning(bool)`：编辑器播放期间置真，使 `Scene` 增删走与 Runtime 一致的**延时路径**（帧末统一增删）——游戏逻辑在迭代中删除对象不再导致容器迭代器失效；停止播放复位
- **Scene 容器迭代安全（引擎）**：`update()` 销毁队列与 `destroy()` 全量清理改为"**先整体搬出容器再逐 clean**"；`processPendingAdditions` 先搬出再处理（`onAttach` 内再增删不进迭代容器）；`removeGameObject`/`removeGameObjectByName` 非运行分支**先摘除再 clean**——`onDetach/onDestroy` 回调内重入增删对象不再迭代器失效/双重清理
- **`GameObject::setScene` 组件遍历（引擎）**：`onAttach` 改为快照遍历 + 生命周期令牌守卫，回调内 `removeComponent`/销毁 owner 不再使 `unordered_map` 迭代器失效
- **音频句柄悬垂（引擎）**：`AudioPlayer::Play` 改为返回 `std::shared_ptr<AudioTrack>`（track 播完引擎只释放自己的引用，调用方持有的句柄不会悬垂）；`AudioTrack` 持 `AudioTrackGroup` 的 `shared_ptr` 令牌，组不会再先于 track 销毁（此前 `AudioPlayer::destroy` 释放组后，仍被外部持有的 track 析构会访问已释放内存）
- **删除相机导致编辑器冻结（编辑器）**：场景树拒绝删除 `scene_camera`/`game_camera`（相机是编辑/运行基础设施，删除后保存无法恢复）
- **项目设置保存旧 SDK 目录（编辑器）**：全局记忆 `lastSdkDir` 改为在 `applyToProject()` 之后取值——此前改 SDK 后「新建项目」向导仍预填旧路径
- **导出资源路径逃逸（编辑器）**：场景 JSON 中 `../` 形式的相对资源路径会写出导出包外且运行时落空——现按绝对路径分支处理（收进包内 `Assets/` 并改写字段），保证导出包自包含
- **日志面板颜色不生效（编辑器）**：`LogWidget::appendMessage` 不再以"选中行"方式上色（无效），改为把字符格式直接应用到插入文本——错误红/警告橙恢复可见
- **示例行为空指针（示例）**：`Player::onUpdate` 对未挂 `TransformComponent` 的对象安全跳过
- **播放中场景缺游戏相机导致双视口冻结（编辑器）**：`EnginePreview::tick` 此前在相机缺失时提前 return——播放态 `ensureCameras()` 补建的相机走引擎**延时添加**（`m_pendingAdditions`），要等 `SceneManager::Update()` 统一 flush 才进场景，提前 return 使补建永不生效（停止播放后滞留的 pending 相机与编辑态再建的相机还会叠加成两个 `game_camera`）。修复：不再提前 return，缺失的相机由各 pass 空守卫跳过、pass 间按名重新定位——补建相机在游戏 pass 的 update 中入场景后当帧生效，游戏逻辑销毁相机后下一帧自愈；顺带消除游戏 pass 内销毁相机后编辑 pass 仍解引用缓存指针的悬垂隐患

### 新增

- **碰撞回调（P19 玩法回路，引擎 + 示例）**：`Behavior` 新增 `onCollisionEnter/onCollisionStay/onCollisionExit(GameObject* other)` 虚方法，由 `PhysicsSystem2D` 在物理步进后按**接触对**驱动（Box2D Begin/End 接触事件 → 刚体对解析 → 定向派发给涉及对象上已启动的 Behavior）：
  - Enter/Stay/Exit 语义基于规范化刚体对集合（`a < b`）判定：新接触 → Enter；上一帧在集合、本帧仍在 → Stay（每帧一次）；结束 → Exit；接触重建/休眠唤醒不重复 Enter
  - 派发安全：回调前逐对校验刚体仍注册、对象仍在场景；`invokeCollisionCallbacks` 逐轮重扫（回调内销毁对象/移除组件不悬垂）；刚体/碰撞体销毁时同步清理接触集合（End 事件 shape 已失效无法自行解析，`destroyRigidBody`/碰撞体卸下兜底清除，防残留泄漏）
  - 碰撞体创建形状时开启 `enableContactEvents`（Box2D 默认关闭，否则无 Begin/End 事件）
  - 示例 `Examples/src/CoinDemo.h`：`BallDemo`（滚动小球打印接触对象）+ `CoinPickup`（金币被碰 → 音效 + 计数 + 销毁自己），配套演示场景 `Examples/scenes/CoinCollect.scene`
- **AudioSource 组件（P19，引擎）**：挂在对象上的音频播放组件（`Audio/AudioSource.h`，继承 Behavior）：反射字段 `AudioPath / Loop / Volume / PlayOnStart` 可序列化进 `.scene` 并由检查器编辑；`play()/stop()/pause()` 运行时控制，`onStart` 按 PlayOnStart 自动播放（全局暂停/编辑态不自动出声）；播放句柄持 `shared_ptr<AudioTrack>` 不悬垂
- **物理调试绘制（P19，编辑器）**：场景/运行视口按相机投影绘制碰撞体轮廓（动态绿 / 运动学黄 / 静态蓝灰 / 无刚体暗灰，随对象旋转），视口内工具条新增「碰撞体」开关（默认开启）——摆物理场景无需盲调
- **导出游戏（P18，引擎 + 编辑器）**：
  - 「文件 → 导出游戏…」：一键把项目装配为**绿色免安装游戏目录**（`<游戏名>.exe` + 引擎/SDL 运行库 + 项目脚本 DLL + 场景 + Assets + config.json），双击即玩
  - Runtime 硬化：`ShitRuntime` 启动时 chdir 到 exe 所在目录（Windows/POSIX）——导出包任意位置运行，不再依赖调用方 CWD；`Runtime/CMakeLists.txt` 新增 install 规则，SDK 自此携带 `ShitRuntime.exe`（导出从 SDK bin/ 取运行库，与仓库解耦）
  - 场景资源路径改写：导出时深度遍历场景 JSON，绝对路径资源复制进包并改写为 `Assets/…` 相对路径；项目根内相对路径按同相对位置带出——解决编辑器拖入资源存绝对路径导致运行包缺图的问题
  - 导出 config.json 自动生成（scene / plugins / inputMappings），输入映射随包带走（引擎本就合并读取 config.json）
- **打开代码编辑器（P16，编辑器）**：
  - 「编辑 → 打开代码…」（`Ctrl+Shift+O`）：用项目设置中配置的 IDE 打开项目根目录
  - IDE 在「文件 → 项目设置… → 通用 → 代码编辑器」下拉中选择：自动探测本机已安装的 Visual Studio Code / Visual Studio（vswhere 查询 + 常见路径回退）/ CLion（Toolbox 与传统安装）/ Qt Creator，也可「浏览…」手动指定任意 exe（存 `config.json` 的 `editor.ideExe`；未配置时菜单给出引导提示）
  - 新建项目（含脚本工程）模板补根 `CMakeLists.txt`（`project(name)` + `add_subdirectory(Scripts)`）：CLion / Visual Studio 打开项目根目录即得完整 CMake 工程；不影响现有构建流程
- **项目设置页（P15，引擎 + 编辑器）**：
  - 编辑器「文件 → 项目设置…」由单字段 SDK 输入框升级为设置对话框（`Editor/projectsettingsdialog.*`）：**通用页**（项目名称 / SDK 目录 / 启动场景下拉选择，写 config.json 的 `scene` 字段）+ **输入页**（动作与轴的按键映射编辑：点击键名按钮进入「按下任意键」捕获窗，支持多键与鼠标键、行内增删、重复键冲突警告后确认保存）
  - 输入映射持久化到项目 `config.json` 的 `inputMappings` 段（与引擎 `settings.json` 同构）；引擎 `Config::init()` 合并读取同目录 `config.json` 的 `inputMappings`（覆盖 settings.json）——独立 Runtime 与编辑器播放共用同一套按键映射；无 config.json 行为不变
  - 编辑器即时生效：打开项目 / 进入播放 / 保存设置时 `Config::loadFromJson` + `Input::InitMappings()` 热重编译（播放中改键立即生效）；新建项目模板默认附带映射（Jump=Space、Sprint=Left Shift、Horizontal=A/D、Vertical=S/W）
  - 键码工具收敛到 `Editor/keys.*`（Qt→SDL 映射自 `mainwindow` 迁出，显示名 / 转发名 / 存储名统一）
- **项目系统（P14，引擎 + 编辑器）**：
  - 编辑器「文件 → 新建项目…」向导（名称/位置/SDK 目录/脚本工程选项）与「打开项目…」（含最近项目列表、启动自动恢复上次项目）；项目配置 `config.json`（`engine.sdkDir` / `plugins` / `scene`，相对路径）
  - 项目私有状态迁移到 `<项目>/.shitengine/state.ini`（Qt IniFormat）：Dock 布局 / 窗口几何 / 最近场景随项目走，无项目回退全局注册表（`lastProjectDir`/`lastSdkDir`/`recentProjects` 仍全局）
  - C++ 脚本工程生成（`Editor/templates/`）：`Scripts/CMakeLists.txt`（`find_package(ShitEngine CONFIG)` + 反射扫描）与 `plugin_export.cpp`/`Behaviors.h` 骨架；「构建脚本」`Ctrl+B` → 异步 `cmake` 配置 + 编译（SDK 编译器自动探测：MSVC → VS 2022/2026 生成器候选逐个尝试，MinGW → Ninja），DLL 输出到项目 `bin/`
  - 热重载（`EnginePreview::reloadProjectPlugins`）：场景 JSON 快照 → 销毁对象（旧 DLL 代码析构完）→ 卸载插件 → 重载 DLL → 注册 → 快照恢复；编辑器现场不清空，Ctrl+B 循环开发无需重启
  - 修复构建覆盖失败（LNK1104-1 文件占用）：脚本工程产物输出到 `build/out/`，构建成功后 `reloadProjectPlugins` 先卸旧 DLL（释放文件锁）→ 回调替换 `bin/` → 再加载新 DLL；构建失败不影响已加载的旧插件
  - 修复移动 Gizmo 无法拖拽（P11 遗留）：`viewport` 的 Move 模式命中检测缺失（`DragMode::GizmoX/GizmoY` 从未赋值）——补 X/Y 轴手柄与中心方块整体拖（`DragMode::Move`）
  - 修复运行视图对象渲染双份：编辑器双 pass 渲染轮流改相机 `enabled`，保存恰好落在禁用态 → `game_camera` 被序列化为禁用 → 下次加载 `ensureDefaultCamera` 兜底新建重复相机 → 同画面渲染两遍。修复：tick 收尾把 `game_camera` 复位为启用；`SceneSerializer` 兜底时若存在同名 `game_camera` 相机则**启用复用**而非新建（旧文件自动自愈）
  - **运行态（Unity 式三态，引擎+编辑器）**：「▶ 运行」先编译脚本（项目含脚本工程且已配 SDK 时）并热加载，完成后进入播放；运行中可自由修改属性/场景（不记录撤销）；「⏸ 暂停/继续」仅冻结/恢复逻辑（画面保持）；「■ 停止」恢复**运行前快照**——运行期的对象/属性改动全部回退（保留编辑器相机）；**每次进入运行从头开始**：`Time::ResetTotalTime()`（时钟归零）、`BehaviorSystem::resetAllBehaviors()`（onStart 运行第一帧重新执行）、`Input::ResetState()`（键鼠快照清空，防卡键）；运行中触发换场景/新建/构建/切换项目/退出等会自动先停止（`openScenePath`/`openProjectPath`/`newScene`/`onBuildScripts` 单点防护）；关于对话框补充运行态说明
  - 引擎 `PluginManager`：config 中的相对 DLL 路径改为以 config.json **所在目录**为基准解析（编辑器 CWD 不可控；Runtime 同目录语义不受影响）
  - 引擎 SDK 完备：`cmake --install` 产物自包含（引擎/依赖 DLL、头文件、`ShitEngineConfig.cmake`、`ReflectionScanner.exe` + `libclang.dll` + libclang resource、`ShitEngineToolsConfig.cmake`）；`ReflectionScanSetup.cmake` 适配 SDK 模式（`REFLECTION_SCANNER_EXE`/`REFLECTION_CLANG_RESOURCE_DIR` 变量）；多配置生成器无 `CMAKE_BUILD_TYPE` 时按 bin/ 探测自动采用 `-d` 后缀
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

- **游戏相机不定名（编辑器，对齐 Unity）** — 运行视口不再锁定名为 `game_camera` 的相机：引擎每帧从场景所有启用相机中自动挑选（priority 最小者优先，上一帧选择延续防抖动），`Main Camera`/`PlayerCam` 等任意命名均可；场景无任何相机时兜底编辑器相机 `scene_camera`，运行视口画面不断流。编辑器相机 `scene_camera` 仍为约定名（不入库、树中隐藏、拒绝删除）。新建场景模板不再预置 `game_camera`；旧场景残留的 `game_camera` 自动兼容为普通游戏相机（可自由删除，运行视图自动改选）
- **场景视图只渲染编辑器相机（编辑器）** — 编辑 pass 渲染期间暂时禁用其余全部相机（渲染后逐一恢复），多游戏相机分屏/画中画不再把各视口内容叠加进场景视图；运行视图 pass 仍渲染全部启用用户相机，多相机 viewport 分屏完整支持（引擎 `RenderSystem` 本就按 `viewportRatio` 逐相机渲染）
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
