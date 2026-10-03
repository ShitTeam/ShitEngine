# ShitEngine 全源码审计与改进路线图

> **审计范围**:`Engine/`(118 文件 / 13,126 行)、`Editor/`(60 文件 / 12,415 行)、`Runtime/`(107 行)、`Tools/ReflectionScanner/`(1,100 行),合计 **185 文件 / 26,748 行**(不含 `external/`、`generated/`、构建产物)。
> **审计方法**:7 路模块级人工通读(逐文件完整读取 + 引擎侧交叉验证防误报)+ 构建/CI/打包横向核查 + 关键结论逐条复现验证。
> **本机工具链**:CMake 4.3.4、GCC 13.1.0(MinGW)、Ninja(CLion 自带)、Qt 6.8.3 MSVC2022(编辑器侧)。
> **状态**:全文完成。§4.1/§4.6 为审计人亲自核查;§4.2–4.5 为模块审计(标注【已复核】的条目已由审计人逐条验证,标注 ⚠回归 者为近期 AnimFrame 改造引入)。

---

## 0. 执行摘要

**审计规模与方法**:185 文件 / 26,748 行;7 路模块审计(每路逐文件完整读取 + 引擎侧交叉验证防误报)+ 审计人对构建/CI/SDK/引擎核心的横向核查,关键结论逐条复现。共产出 **95 条代码级缺陷**(Critical 3 / High 19 / Medium ≈30 / Low ≈43)+ **12 条构建/CI/打包问题** + **21 项功能缺口** + **11 条横向架构观察**。

**总体判断**:这是一个**设计意图清晰、实现密度高**的 2D 引擎——"场景数据驱动 + 编译期反射 + 进程内嵌编辑器"三条主线贯穿一致,多上下文(`EngineContext`)、组件 UUID 引用、固定步/变步长双循环等决策方向正确;大量历史踩坑处都有扎实的防御(延迟删除、墓碑化、快照回滚、环检测、generation 校验),且多处注释明确记录了教训。核心循环(Time/EventBus/Input/Game)、物理系统(PhysicsSystem2D)、渲染系统排序/剔除**经逐文件核查未发现显著缺陷**。

**但存在三类系统性风险**:

1. **工程门禁缺失**(最危险):CI 只做 4 平台编译 + 打包,**不校验反射代码一致性**,且生成的 `.gen.h` 与构建 stamp 文件全部入库。"改了 `SHIT_REFLECT` 头而忘记重生成"或"扫描器部分解析失败"(S4)**CI 永远绿**,后果是 `FieldInfo` 偏移与实际结构错位 → 按错误偏移 `memcpy`(**内存破坏级**)。项目**零测试**,26.7k 行代码全靠人工验证。
2. **发布产物不可信**:SDK 配置模板仍按**已废除的 `-d` 后缀约定**查找引擎 DLL,MinGW 分支还多期望一个 `lib` 前缀;实测 SDK 里同时躺着新旧两份引擎 DLL——消费者会**静默链接到旧引擎**。根因是"安装目录从不清理"(§1.1/1.4)。
3. **双真值/载体的不变量靠各调用点自律,已实际失联**:反射层不支持容器 → "字符串载体"与"双字段真值"模式蔓延,而不变量靠散落各处的约定维护——`Tilemap::m_gridData` 标了 `readOnly` 导致**每次保存丢全部瓦片**(S1,Critical);本次审计中近期 `AnimationClip` `frames`/`frameSprites` 双模型也被抓出 3 条回归(D1/D2/D3)。**这不是巧合,是同一病根(§2.1)的两个症状。**

**最该先做的三件事**:
1. **修 S1(一行)**:`Tilemap::m_gridData` 去掉 `.readOnly = true` —— 当前每次保存丢全部瓦片数据,且与模块自身注释直接矛盾。
2. **修 D1/D2/D3(本次 AnimFrame 改造的回归)**:编辑器读侧统一到 `frameSprites` 单一真值(原计划 Phase 2)——新格式 `.anim` 当前在编辑器里整体失效、拖帧追加清空原动画、拖动排序写回旧顺序。
3. **修 CI 盲区 + 引入测试**:CI 加反射一致性校验 + 从库中移除 stamp + 引入测试框架接入 CI。三者低成本、高杠杆,不触碰架构。

**缺陷分布速览**:Critical 3(S1 瓦片数据丢失;D1/D2 AnimFrame 回归)· High 19(插件卸载 UAF、检查器输入被抹、批量删除误删、播放中存运行态、序列化 size 守卫缺失、扫描器静默失败、类型名覆盖、assetPath 绝对路径、导出丢 .anim 依赖、tileset 旧控件残留、精灵表拖拽失效等)· 其余 Medium/Low 见 §4 各表。

**架构层最重要的三项**(详见 §2/§5):① 反射层容器支持(回报最高的引擎重构,顺带消灭载体模式与双真值);② 资产数据库(GUID + 导入管线,否则移动资产即断链);③ 渲染后端抽象(先抽接口不换后端,拿到解耦收益)。

---

## 1. 构建 / CI / 打包(全部逐条验证)

| # | 严重度 | 问题 | 证据 | 影响 |
|---|--------|------|------|------|
| 1.1 | **High** | **SDK 导出包在 Debug / MinGW 下不可用** | `Engine/CMakeLists.txt:90-97` 统一产物名为 `ShitEngine.dll`(注释明言命名漂移会让插件 `LoadLibrary` 报 126);而 `Engine/cmake/ShitEngineConfig.cmake.in:15-19,99-104` 仍按 `-d` 后缀与 MinGW `lib` 前缀查找:`ShitEngine-d.dll` / `libShitEngine.dll` | 消费者按 SDK 的 CMake 配置链接时找不到引擎 DLL(导入目标无 `EXISTS` 兜底,直接报错);若目录里恰有陈旧同名文件则**静默链到旧引擎** |
| 1.2 | **High** | **CI 不校验反射代码一致性** | 4 个平台 job 全部 `-DBUILD_TOOLS=OFF`(`.github/workflows/build.yml:49,102,166,214`);`Engine/generated/reflection/*.gen.h` 已入库(`git ls-files` 32 项) | 改 `SHIT_REFLECT` 头而忘记重生成 → CI 全绿,但 `FieldInfo` 的 offset/size 与实际结构错位 → 编辑器/序列化按错偏移 `memcpy`,属内存破坏,且**不会被任何自动化拦住** |
| 1.3 | **High** | **构建 stamp 文件入库** | `.reflect-engine-stamp`、`.reflect-stamp` 被 Git 跟踪(生成逻辑见 `Tools/ReflectionScanner/ReflectionScanSetup.cmake:116,124-131`) | 全新 clone 时 stamp 与头文件同批 checkout,mtime 关系不确定 → Ninja 可能判定"输出已最新"而**跳过扫描、沿用陈旧生成代码**;本次审计即遭遇 Ninja 因 glob/stamp 状态卡死(需删 `build.ninja` 重配置才恢复) |
| 1.4 | **Medium** | **安装/输出目录从不清理** | 实测 `out/sdk-msvc/bin/` 同时存在 `ShitEngine.dll`(8/20 16:40)与 `libShitEngine.dll`(8/20 12:23);`out/build/x64-debug/bin/` 还留着已删除模块的 `SceneDumper.exe` | SDK 夹带陈旧二进制与废弃可执行文件 → 用户拿到"看起来正常但内核是旧的"的包(与 1.1 叠加放大危害) |
| 1.5 | **Medium** | **三处文档/警告给出不可用命令** | `CMakeLists.txt:40`、`AGENTS.md:26/54/246`、`Editor/ROADMAP.md:69/306` 均让 `BUILD_TOOLS=OFF` 的用户执行 `cmake --build . --target run-reflectionscanner`;但该目标只在 `if(BUILD_TOOLS)` 内创建(`CMakeLists.txt:66-84`) | 按提示操作得到 "unknown target",用户被迫自行摸索;正确做法是先 `-DBUILD_TOOLS=ON` 重新配置 |
| 1.6 | **Medium** | **Release 创建存在竞态** | `.github/workflows/build.yml:73-74,129-130,194,240` 四个平台 job 各自 `gh release view \|\| gh release create`(无正文);`.github/workflows/release.yml:44-46` 另做一次 `gh release create --notes-file`(从 CHANGELOG 抽取) | 并发下先创建者决定 Release 正文:若平台 job 抢先创建,**CHANGELOG 正文整段丢失**且后续 `--notes-file` 创建直接失败 |
| 1.7 | **Medium** | **工程零测试** | 全库无任何测试文件;CI 无 `ctest`/test 步骤(`.github/workflows/*.yml` 仅编译+打包) | 26.7k 行引擎 + 12.4k 行编辑器无回归网;每次改动靠人工验证,历史 BUG 修复(`55a4259` 6 处)全靠通读发现 |
| 1.8 | **Low-Med** | **子目录 CMake 最低版本过低且版本号不一致** | `Editor/CMakeLists.txt:1` 声明 `VERSION 3.5`(根为 3.20,Scanner 为 3.20);本机 CMake 已是 **4.3.4**;同文件 `:3` `project(Editor VERSION 0.1)` 与引擎 1.4.2 不一致 | CMake 4.x 正在移除 <3.10 兼容,未来直接报错;Editor 版本号会让 macOS bundle 元数据写着 0.1.0 |
| 1.9 | **Low** | **Runtime 后置拷贝与注释不符** | `Runtime/CMakeLists.txt:17` 注释称拷贝 `config.json / settings.json / resource/ / Scenes/*.scene`,实际只拷 `config.json` 与 `Scenes/`;`Runtime/settings.json` 在源码树中根本不存在 | 注释误导;当前 `Runtime/config.json` 只引用 `Scenes/` 故未暴露故障,一旦引用 `resource/` 即会在干净构建上加载失败 |
| 1.10 | **Low** | **CMakePresets 硬编码本机路径** | `CMakePresets.json:11-13` 写死 `D:/CLion/bin/ninja/win/x64/ninja.exe` 与无路径 `gcc/g++`;`x86-debug/x86-release` 预设缺 `BUILD_TOOLS` | 换机器/换工具链即配置失败;预设本应是可共享资产 |
| 1.11 | **Low** | **依赖管理三处脆弱** | `Engine/cmake/DependencyManager.cmake:45` `find_package(... QUIET)` **无版本约束**(系统装 SDL3 3.2 会静默顶替期望的 3.4.8);`:25` 用 `macro` 定义导致 `_LIB_IS_SHARED`/`USE_SHALLOW` 等内部变量泄漏到调用方作用域;`:128,136,175,187` 以 `CACHE ... FORCE` 改写再"还原" `BUILD_SHARED_LIBS`,易产生缓存漂移;本机 `external/` 命中时不校验版本 | 依赖版本与链接类型可能在不同机器上静默不同,难以复现问题 |
| 1.12 | **Low** | **总入口头缺 `EngineContext.h`** | `Engine/include/ShitEngine.h` 未包含它,且 `grep` 确认 `Engine/include/` 下**没有任何头文件**包含 `EngineContext.h` | 文档化的多实例公开 API(`ShitEngine::EngineContext preview;`)无法经唯一入口 `#include <ShitEngine.h>` 使用,与"消费者统一入口"的定位矛盾 |

---

## 2. 横向架构问题

### 2.1 反射层不支持容器 —— 多项缺陷的共同病根
`FieldInfo` 只按 `offset + size` 做 `memcpy`,不支持 `std::vector`/数组/嵌套类型,于是所有可变长数据都退化成**「反射 `std::string` 载体 + `onAfterDeserialize`/`onFieldChanged` 手工解析」**模式(`Tilemap::m_gridData`、`Animator::m_animatorData`、`AnimationComponent::m_clipsData`)。

这是**当前最值得优先解决的架构瓶颈**,理由:
- 每个用到它的组件都要手写「解析/反向同步/长度校验」三套代码,重复且易漏(本次刚修的 `AnimationClip` `frames` vs `frameSprites` 双模型不同步即此模式的直接产物);
- 编辑器无法原生编辑这类字段(只能给个裸 JSON 字符串框);
- 序列化往返丢失的风险点全部集中在这里。

**建议**:给反射层加"容器描述符"(`FieldKind = Scalar|String|Enum|Ref|Vector<T>|Array<T>` + `elementType`/`elementSize`),让序列化器与检查器原生处理 `std::vector`,载体模式随之退役。**这是回报最高的一项引擎重构。**

### 2.2 没有资产数据库:路径即引用
`.scene`/`.anim`/`.prefab` 里存的是**文件路径字符串**。移动或重命名资产 → 全项目断链,且无任何检测手段;无导入设置(纹理过滤/压缩/切片参数无处安放)、无依赖图(unused/缺失资产无法统计)、无热重载。
对照:Unity 用 `.meta` + GUID 做资产标识与依赖追踪(`AssetDatabase`),Godot 用 `ResourceUID` + `.import` 侧车文件。

### 2.3 序列化无版本校验
`SceneSerializer::toJson` 写入 `"version"`(`SceneSerializer.cpp:181,246`),但 `fromJson`(`:270-297`)**从不读取或校验**它——`version` 是只写元数据。未来格式升级时,旧版本引擎会**静默误解析**新文件,而不是明确拒绝。文件级与组件级都缺少迁移(migration)钩子。

### 2.4 渲染后端锁定 SDL_Renderer
`Renderer.cpp:20` 用 `SDL_CreateRenderer`,绘制走 `SDL_RenderTexture`/`SDL_RenderTextureRotated`(`:119,161,168`)。这意味着:**无法自定义 Shader、无后处理、无法控制批处理与混合状态**。对一个要做像素风 + 特效的 2D 引擎,这是明确的长期天花板。
对照:Godot 自建 `RenderingServer`(节点与渲染服务解耦)、Unity 有 SRP。
**建议**:不必立刻换后端,但应**先抽出渲染接口层**(`IRendererBackend`),把"绘制精灵/设置视口/清屏"抽象出来,未来接 SDL_GPU/bgfx 时不动上层。

### 2.5 插件 ABI 只做到了"入口是 C"
`PluginManager.h:23-29` 有 `kAbiVersion = 2` 与 `GetPluginABIVersion()` 校验(好),入口也是 C 函数指针(好)。但插件**静态链接引擎 DLL**、跨边界使用 C++ 类型/STL/异常/RTTI,因此并非稳定 ABI:引擎任何头文件布局变化都可能让旧插件静默错乱。`AGENTS.md` 里"引擎 DLL 命名漂移会让插件 `LoadLibrary` 报 126"这条约定,本质就是这个脆弱性的症状。
对照:Godot 的 GDExtension 定义了**版本化的稳定 C ABI** + 能力协商,扩展可跨引擎小版本使用。

### 2.6 多上下文隔离存在未声明的静态例外
项目的核心设计是"所有子系统按 `EngineContext` 隔离",`AGENTS.md` 明确声明 `Log` 是**唯一**例外。但 `PluginManager::GetLastLoadError()`(`PluginManager.h:53`)是 `static` 的**跨实例共享状态**——两个上下文(编辑器主上下文 + 预览上下文)会互相覆盖错误信息。

### 2.7 编辑器与引擎的同步模型偏重
- 检查器**每帧从引擎全量回读**并重建控件;重建会打断输入焦点与滚动位置,且随字段数增长线性变慢。对照:Unity 用 `SerializedObject` + 脏标记(`Undo.RecordObject`)、Godot 用 `EditorInspector` + `_get_property_list`。
- 撤销是**整场景快照**对比(`undostack.*`):每次编辑序列化整个场景做 diff,场景一大就明显变慢;且无法做字段级合并(连续拖拽只能合并成一次)。
对照:Godot `UndoRedo` 按动作记录、Unity 按对象差异记录。

### 2.8 `mainwindow.cpp` 职责过载
`Editor/mainwindow.cpp` **2,152 行**,是第二大文件(`inspector.cpp` 1,384)的 1.5 倍,也是编辑器其余文件均值(约 250 行)的 8 倍以上,同时承担:菜单/快捷键、场景的保存-打开-回滚、播放态生命周期、Dock 组织、资产拖放、动画窗口联动、输入转发、导出流程。建议按"场景文档(persistence)+ 播放控制(playmode)+ 面板协调(docks)"拆分为三个协作类。

### 2.9 没有性能剖析设施
公共头文件中**没有任何**帧时间/系统耗时/绘制调用统计 API(`grep Profiler|GetFPS|DrawCall|Statistics` 零命中)。项目已具备"固定步 + 变步长 + 多系统优先级"的复杂调度,却无法回答"这一帧谁慢"。对照:Unity Profiler、Godot Debugger → Monitors。
**建议**:引擎侧收集(每系统 update/fixedUpdate 耗时、绘制批次数、物理步耗时),编辑器加覆盖层——这也是后续所有性能优化的前提。

### 2.10 全同步、单线程
引擎自身不创建线程(`grep std::thread|SDL_CreateThread` 仅命中 `EventBus` 的互斥锁与 spdlog sink)。资源全部同步加载 → 大纹理/音频会直接卡帧;无作业系统。对照:Unity `Addressables`/`AsyncOperation`、Godot `ResourceLoader.load_threaded_request`。
注:SDL_mixer 的回调线程与引擎资源管理是否有数据竞争,属模块级待确认项(见第 4 节)。

### 2.11 场景模型为单一当前场景
`SceneManager` 只有"当前场景 + `LoadSceneFromFile` 替换",**无附加加载(Additive)/流式/异步**,关卡切换必然整帧卡顿,也无法做"常驻管理器场景 + 关卡场景"这种业界标准分层。

---

## 3. 功能缺口清单(证据化:对 `Engine/include` 全量 grep 零命中)

| 能力 | 现状 | 对照(Unity / Godot) | 优先级 |
|------|------|----------------------|--------|
| **粒子系统** | 缺失 | ParticleSystem / GPUParticles2D | **高**(2D 游戏刚需) |
| **碰撞层与掩码** | 缺失 —— 物理组件完全未暴露 `categoryBits`/`maskBits` | Layer Collision Matrix / `collision_layer`+`collision_mask` | **高**(玩家/敌人/地形分组是基本需求) |
| **物理查询(射线/重叠)** | 缺失 | `Physics2D.Raycast` / `intersect_ray` | **高** |
| **性能剖析** | 缺失 | Profiler / Debugger Monitors | **高** |
| **测试框架** | 缺失 | Test Framework / GUT | **高** |
| **Shader / 后处理** | 缺失(SDL_Renderer 限制) | Shader Graph / ShaderMaterial | 中高 |
| **动画事件** | 缺失(Animation 头无 event/blend/layer) | AnimationEvent / method track | 中高 |
| **相机跟随/边界/多目标** | 缺失(仅静态相机 + zoom/viewport) | Cinemachine / Camera2D limits | 中高 |
| **Tilemap 自动地形 + 瓦片碰撞** | 缺失(仅网格铺排) | RuleTile / Terrain autotile | 中 |
| **动画混合树 / 动画层** | 缺失(仅单层状态机) | BlendTree+Layers / AnimationTree BlendSpace | 中 |
| **UI 布局容器 / 滚动视图** | 缺失(仅锚点定位) | LayoutGroup+ScrollRect / Container+ScrollContainer | 中 |
| **补间 / 协程 / 计时器** | 缺失 | DOTween(社区) / Tween | 中 |
| **精灵图集打包** | 缺失 | Sprite Atlas / AtlasTexture | 中 |
| **Prefab 变体 / 嵌套覆盖** | 缺失(仅捕获+实例化) | Prefab Variant + Overrides | 中 |
| **运行时存档** | 缺失 | JsonUtility/存档系统 / `ResourceSaver` | 中 |
| **构建管线(场景列表/资源打包)** | 部分(有导出器,无场景清单/资源压缩) | Build Settings / 导出模板 | 中 |
| **场景附加加载 / 流式** | 缺失 | Additive Scenes / `change_scene` 分层 | 中 |
| **音频总线效果 / 流式播放** | 缺失(仅分层增益) | AudioMixer / AudioBus effects | 中 |
| **运行时改键 / 多设备** | 部分(设置页写 config,需重启) | Input System / InputMap 运行时改 | 中 |
| **本地化** | 缺失 | Localization / TranslationServer | 低 |

---

## 4. 代码级缺陷清单(模块审计汇总)

> 每条均带 `文件:行号` 与后果。§4.1 为审计人亲自逐文件核查的引擎侧发现;§4.2 起为模块审计结果(关键条目已由审计人亲自复核)。

### 4.1 引擎侧(审计人亲自核查)

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| E1 | **High** | API | `Engine/src/ShitEngine/Render/Renderer.cpp:65-87` | `readPixels(void* pixels, int pitch)` **无缓冲区大小参数、无行数上限钳制**,把整个渲染目标逐行拷入调用方缓冲 | 当前两个调用点(`Editor/preview.cpp:360,394`)恰好在 `BeginOffscreen/EndOffscreen` 括号内、且 `m_pixels` 按同一逻辑尺寸一次性分配(`preview.cpp:89`)而侥幸安全;`Renderer` 无 `SetLogical` 运行时接口所以离屏目标不会错位。但任何未来调用方、或任何尺寸错配(目标为后备缓冲时读到**窗口尺寸**)即**堆溢出** | API 改为带 `out w/h` 或加 `maxRows` 钳制;断言 pitch 与目标宽度一致 |
| E2 | **Medium** | Bug | `Engine/src/ShitEngine/UI/UIText.cpp:98,107-134` | 控件自持裸 `SDL_Texture* m_cachedTexture`(经 `CreateTextureFromSurface` 创建,归属当前 SDL_Renderer),**无渲染器重建失效机制** | `RenderSystem` 每帧刷新 `m_renderer` 指针正是为了防"渲染器重建后悬垂"(`RenderSystem.cpp:40-41`),而 `UIText::onRender` 在渲染器重建后直接用缓存纹理 → **潜在 UAF**。今天难触发(场景随 EngineContext 同亡),但缺口真实 | UIText 持渲染器代数/指针,`onRender` 发现不匹配即重建纹理 |
| E3 | **Medium** | Bug | `Engine/src/ShitEngine/Component/Tilemap.cpp:108-126` | `onRender` 对 `tileId` **无上界校验**:`src.y = (tileId / tilesPerRow) * m_tileHeight` 可超出纹理高度 | 手改/损坏的 `m_gridData` 含越界 id → 每帧对每个越界瓦片调 `SDL_RenderTextureRotated`,SDL3 拒绝并置错 → **每帧错误日志刷屏** + 瓦片缺失 | 渲染前钳制 `tileId` 到 `tilesPerRow * (texH / m_tileHeight)` |
| E4 | **Medium** | Bug | `Engine/src/ShitEngine/Component/SpriteRenderer.cpp:61-71` | `setTexturePath` 失败时保留 `m_texturePath`(新路径)但**不更新** `m_sprite`(旧路径) | 两字段分叉:`onRender` 用 `m_sprite`(渲染旧纹理)、序列化用 `m_texturePath`(存新路径)——"看到的是 A、存的是 B";且每次重试都打 ERROR | 失败时同步 `m_sprite` 路径(渲染空)或引入单一真源 |
| E5 | **Low-Med** | 性能 | `Engine/src/ShitEngine/Component/SpriteRenderer.cpp:17` + `Renderer.cpp:93,96` | `onRender` 每精灵每相机每帧做 `ResourceManager::Load<Texture>` 查表(字符串哈希 + unordered_map);`DrawSprite` 纹理缺失时**每帧打 ERROR** | 千精灵场景每帧数千次查表 + 日志刷屏 | 组件缓存 `Texture*`(`setTexturePath` 时失效);失败日志降频(每路径一次) |
| E6 | **Low-Med** | 架构 | `Engine/src/ShitEngine/UI/UITextInput.cpp`(全文件) | 文本输入**无剪贴板支持**(全 UI 模块 grep `clipboard` 零命中):无 Ctrl+C/V/X/A | 聊天栏/登录框等任何文本输入都无法粘贴——对带文本的游戏是硬伤 | 接入 `SDL_SetClipboardText/GetClipboardText` |
| E7 | **Low** | Bug | `Engine/src/ShitEngine/Render/RenderSystem.cpp:57-59` vs `UIRenderSystem.cpp:25` | 游戏世界排序用 `std::sort`(不稳定),UI 用 `std::stable_sort` | 同 `zIndex` 精灵的相对顺序在每次重排时可能跳变(新组件注册触发重排)→ 视觉闪烁 | 统一用 `stable_sort`(同 zIndex 保持注册序,Unity 语义) |
| E8 | **Low** | 架构 | `Engine/src/ShitEngine/Core/TextInputGate.h:14` | 头注释自称"**进程级**单例",实现(`TextInputGate.cpp:15-16`)实为 `EngineContext::current()` **每上下文一份** | 与多上下文设计矛盾的是**注释**而非代码——但会误导后来者按进程级语义改代码 | 注释改为"每 EngineContext 一份" |
| E9 | **Low** | 性能 | `Engine/src/ShitEngine/UI/UIRenderSystem.cpp:38,72,118` | `visible` 缓冲每帧重建(vector 新分配);按钮/渲染阶段对每条目做 `std::find(m_uiRenderers...)` 全表扫(O(n²)/帧) | 控件多时每帧开销线性放大 | `visible` 改成员 + `reserve`;身份校验用 set 或登记标志 |

**核查为安全、勿改**的易疑点(审计人已逐条验证):`Scene` 的 System 移除走延迟队列(`processPendingRemoveSystems`,连 vector 重分配与悬垂 key 都考虑到了);固定步夹取与暂停语义站得住(`Scene.cpp:69-82`);`GameObject::setParent` 有环检测(`GameObject.cpp:85-88`)故 `UITransform::resolveParentRect` 递归安全;`UITextInput` UTF-8 字节游标/多字节步进正确;`AudioPlayer` 全失败路径 `MIX_DestroyTrack` + 组生命周期 shared_ptr 令牌;`Game::destroy()` 销毁顺序与标志复位正确。

### 4.2 编辑器主窗口 / 项目 / 预览(模块审计,关键条目已复核)

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| M1 | **High** | Bug | `Editor/mainwindow.cpp:1201-1206` | **播放态 Ctrl+S 可直接保存**:`saveScene` 无 `isPlaying` 检查(对比 `newScene:543`/`openScene:574`/`closeProject:1604`/`closeEvent:1550` 都先停)【已复核】 | 播放中 Ctrl+S → 物理瞬态位置、运行中生成/销毁的对象被序列化写盘;停止后恢复 `m_runSnapshot` 又标 dirty,用户把运行前状态再存一次或误判编辑丢失 | `saveScene` 开头加同款 `if (isPlaying()) setPlaying(false);` |
| M2 | **High** | Bug | `Editor/preview.cpp:126-147`(loadProjectConfig)与 `:149-157`(unloadPlugins) | **切/关项目缺"卸载前清理插件注册的系统"**:`reloadProjectPlugins:194-204` 已有完整 2.5 段(getRegisteredSystemTypeNames + unregisterSystem + flushPendingSystemRemovals),这两条路径只有 `clearSceneObjects + UnloadAll`【已复核】 | 场景挂了插件 System → `UnloadAll` 释放 DLL 后 `Scene::m_systems` 悬垂 → a) 关项目下个 tick `Scene.cpp:89 m_systems[i]->update()` 虚调用进已释放模块;b) 切项目 fromJson→syncSceneSystems→`Scene.cpp:495 typeid(*sys)` 解引用已释放对象 → **UAF 崩溃** | 两条路径复用 reloadProjectPlugins 的 2.5 段 |
| M3 | **High** | Bug | `Editor/mainwindow.cpp:1032` + `Prefab.cpp:91,103` | **instantiatePrefab 的 `fromJson` 无 try/catch**(对比 `openScenePath:626`/`rollbackScene:677` 均有);`fieldFromJson` 直接 `j.get<float>()` 无类型检查【已复核】 | 双击/拖入手写或损坏的 `.prefab`(如 `"m_speed": "1.5"`)→ nlohmann 异常逃出槽 → `std::terminate`,编辑器整体崩溃;场景已半实例化、`undoBegin` 事务悬空 | 包 try/catch + 失败回滚;或 `fieldFromJson` 加类型校验 |
| M4 | Medium | Bug | `Editor/mainwindow.cpp:2011-2016` | `eventFilter` 组合键检查在 KeyPress/KeyRelease **共用**——按住 W → 按 Ctrl(不转发)→ 松 W(KeyRelease 带 Ctrl 修饰)被丢 | SDL 收不到 KEY_UP → 引擎 `IsKeyPressed(W)` 整个播放会话保持 true(游戏一直移动),直到停止 | 仅对「KeyPress 且带 Ctrl」跳过;Release 照发 |
| M5 | Medium | Bug | `Editor/mainwindow.cpp:304-306/1488` + eventFilter | `grabKeyboard` 后不处理 `QEvent::FocusOut/WindowDeactivate`——播放中按住键 Alt+Tab 切走,Qt 不补发 KeyRelease → 键永久粘住直到停止;切「场景视口」标签页时视口 hide、键盘 grab 隐藏释放且无重新 grab(待确认) | 游戏键盘输入失联/按键粘住 | eventFilter 处理 FocusOut/WindowDeactivate:对已按下键补发 KEY_UP;视口 show 时重新 grab |
| M6 | Medium | Bug | `Editor/mainwindow.cpp:1028-1035` + `Scene.h:162` | 播放中 `createGameObject` 进 `m_pendingAdditions`,`before` 对比找不到新对象 → `created=nullptr` | 播放中拖入/双击 `.prefab`:落点定位与选中全部跳过 → 预置体落到序列化原位置,无反馈 | 对象查找覆盖 pending,或延迟到下一帧 sync 后定位 |
| M7 | Medium | Bug | `Editor/preview.cpp:91-92` + `mainwindow.cpp:1776-1796` | 模态对话框 `exec()` 期间嵌套事件循环照常触发 QTimer → 引擎 tick、`SceneManager::Update`、`onSceneFrameReady` 全部继续 | 播放中打开「导出游戏…」→ 游戏在对话框后面继续跑,导出器在游戏运行时读写场景与 DLL | 模态对话框前 `SetPaused(true)`/`m_timer.stop()`,结束后恢复 |
| M8 | Medium | Bug | `Editor/mainwindow.cpp:2030-2040` + `keys.cpp:5-47` | 侧键漏转发:输入页支持 XButton1/XButton2 绑定(`Input.cpp:127-128` 可解析),eventFilter 只映射 Left/Middle/Right;`keys.cpp` 缺 `Qt::Key_Plus`(小键盘 +)等 | 输入页配置的侧键动作播放中永不触发,无提示 | eventFilter 补 XButton1/2;keys.cpp 补缺失键 |
| M9 | Low | Bug | `Editor/mainwindow.cpp:1103-1106` | 无项目且 texturePath 相对时拼出 `/xxx.png`(Windows 当前盘根绝对路径) | 纹理找不到 → tilesPerRow 反推兜底可能错位;路径存盘引擎加载失败 | 无项目时用 `applicationDirPath` 拼接(与 `AssetPaths::toAbsolute` 同语义) |
| M10 | Low | Bug | `Editor/logwidget.cpp:68-71` | `m_defaultDir` 空时拼出 `/log_editor_xxx.txt`(盘根) | 「保存日志…」落在盘根 | initial 空时仅传文件名 |
| M11 | Low | Bug | `Editor/mainwindow.cpp:543/574/1604` | 先 `setPlaying(false)` 后 confirmDiscardChanges | 用户在「未保存」弹窗点「取消」后播放已被不可逆停止(与 Unity 相反——Unity 确认后才退) | 先确认再停,或取消时恢复播放 |
| M12 | Low | Bug | `Editor/mainwindow.cpp:1711` | 读 `m_settings.value("projectDir")`——该键仅 AssetsDock 写(`assetsdock.cpp:274`),mainwindow 写的是 `lastProjectDir:1683` | 关项目后资源面板仍浏览刚关闭的项目目录 | 统一键名常量、明确回退语义 |
| M13 | Low | Bug | `Editor/mainwindow.cpp:1663-1680` | 项目场景载入失败时 `m_scenePath` 残留旧路径 | 标题栏显示「旧场景名 - 新项目名」 | 进入时先 clear |
| M14 | Low | Bug | `Editor/mainwindow.cpp:715`(undo:742/redo:754/exitPlayMode:1526) | 撤销/重做/停止恢复路径的 `fromJson` 无 try/catch,防御不一致 | 快照含坏值时 terminate 且栈条目已弹走(概率低) | `applySnapshot` 包 try/catch + 失败回滚 |
| M15 | Low | 性能 | `Editor/undostack.h:43-50` | 撤销栈无深度上限,每条目存全场景 JSON before+after 两份 | 千对象场景连续编辑内存无界增长 | 加深度上限或 JSON diff(patch) |
| M16 | Low | Bug | `Editor/componentmenu.h:46` / `systemmenu.h:45` | 每次调用 `new QMenu(parent)` 无 `WA_DeleteOnClose` | 每次右键在父对象上累积一个 QMenu 子对象(会话期内累积) | `setAttribute(Qt::WA_DeleteOnClose)` |
| M17 | Low | Bug | `Editor/project.cpp:78-171` | 失败路径不回滚:目录骨架已写盘后失败,`m_valid=false` 但磁盘残留半成品 | 下次同名 root 被 `mainwindow.cpp:1573`「目录已存在」挡住 | 失败时删除已创建目录 |
| M18 | Low | 性能 | `Editor/preview.cpp:360-398` + `mainwindow.cpp:1157-1158` | 每 tick 两帧 QImage deep copy(60fps→120 次/秒全帧拷贝)+ 检查器/AnimatorDock 每帧无条件全量反射回读 | 大逻辑分辨率下 ~1GB/s 内存带宽 + 每帧 UI 回读开销 | 双缓冲/move;回读改播放态或代数变化时 |

**该模块架构观察**(要点):① `mainwindow.cpp` 2152 行职责过载,建议拆 PlayModeController / SceneSyncController / SceneFileService / ProjectController / DragDropHandler / PickService 六块;② 撤销栈全量快照(每条目 before+after)对比 Unity `Undo.RecordObject` 命令对象与 Godot `EditorUndoRedoManager` 命令式闭包;③ 无资源数据库(AssetsDock 是 QFileSystemModel 文件浏览器,移动资源即断链,对比 Unity AssetDatabase GUID 重定向);④ 编辑器引擎耦合靠 `EngineContext::current()` 全局指针的副作用(`preview.cpp:31/110` start 后从不恢复,各处显式 setCurrent 是在补洞),建议 RAII 上下文切换;⑤ 热重载失败路径静默丢插件组件(`preview.cpp:207-217` 恢复时旧 DLL 已卸载,`Prefab.cpp:230-233` 对未注册类型仅 WARN);⑥ QSettings 双目标易错(#12 即例证);⑦ 播放态语义与 Unity 对齐良好,唯「先停后问」(#11)相反。

### 4.3 对象模型 / 序列化 / 反射 / 扫描器(模块审计,Critical 已亲自复核)

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| S1 | **Critical** | 序列化 | `Engine/include/ShitEngine/Component/Tilemap.h:84` + `Prefab.cpp:165` | **`m_gridData` 载体字段标了 `.readOnly = true`,而 `Prefab::Capture` 对 readOnly 字段直接 `continue`**(已亲自复核两处)【已复核】 | `.scene`/`.prefab` 保存均经 `Prefab::Capture` → **刷好瓦片的 Tilemap 保存后 `m_gridData` 根本不落盘** → 重载 `parseGridData()` 收到空串 → 全部瓦片归 -1,**每次保存丢全部瓦片**。`Animator.h:151`/`AnimationComponent.h:109` 的注释明确记录"readOnly 字段会被 Prefab 序列化跳过"——作者知道这个坑,Tilemap 仍踩 | 去掉 `m_gridData` 的 `.readOnly = true`(一行);或让 Capture 对序列化载体字段豁免 readOnly |
| S2 | **High** | Bug | `Engine/src/ShitEngine/Scene/Scene.cpp:203-208` + `Prefab.cpp:220-222,283-285` | `Scene::instantiate`(文档化公开入口 `GameObject.h:21`)走 `prefab.apply()`(restoreUuid=**true**,恢复记录 UUID),与 `Prefab::instantiate`(restoreUuid=false,"防跨实例引用串线")语义相反 | 运行时对同一 prefab 两次 `scene->instantiate`:第二次 UUID 撞车被重发,实例内 ComponentRef 仍是记录 UUID → **实例 2 的引用解析到实例 1(或源)的组件,游戏逻辑改错对象**。当前仓库内无调用方(编辑器走 `Prefab::instantiate`),属公开 API 陷阱 | `Scene::instantiate` 改调 `applyInternal(go, false)` |
| S3 | **High** | Bug | `Engine/src/ShitEngine/Scene/SceneSerializer.cpp:36,60` | 系统字段路径的 `tn == "int"` 分支**无 `field.size == sizeof(int)` 校验**(Prefab 路径有,`Prefab.cpp:51`);而 `Scanner.cpp:264-266` 自述 libclang 会把解析失败的模板字段拼写退化为 "int" | 插件 System 子类含被退化的模板字段(WhiteList 下标记、拼写退化 "int"、真实 size 24)→ 加载时向 24 字节对象内部写 4 字节 → **破坏 std::vector 内部指针 → 堆破坏/崩溃**;保存侧把指针低 4 字节当 int 写进 JSON | 两条路径统一加 size 守卫(并抽公共 FieldCodec,见 §4.3 架构观察 1) |
| S4 | **High** | Bug | `Tools/ReflectionScanner/src/Scanner.cpp:490-495` + `main.cpp:125-131` | 单个头文件解析失败 → 该类型不进结果,但 `ReflectionRegisterAll.h` 会被**重写**为不含它的版本;只有"全部失败"才 exit 1,部分失败 exit 0 | 一个反射头 libclang 解析失败(MSVC 专有语法等)→ 构建照常成功 → 该类型所有组件**静默失去反射注册** → `.scene` 保存丢组件、编辑器不可见。与 AGENTS.md 记录的"Ninja 漏 TU 丢字段"同症状,是最难排查的一类 | 解析失败使扫描器非零退出(或 register-all 保留上次成功条目并 WARN) |
| S5 | **High** | API | `Engine/src/ShitEngine/Reflection/TypeRegistry.cpp:12-29` | 注册键是**不带命名空间的裸类型名**;重名时 `*existing = std::move(info)` **静默覆盖**(无告警) | 插件类与引擎组件同名:引擎 TypeInfo 被插件版替换 → `Get(typeid(引擎组件))` 返回 nullptr → **.scene 保存丢该组件**;`Get("名字")` 返回插件版 → 按名实例化出**插件的类**(串线) | 键用命名空间限定名,或对不同 typeIndex 的同名注册报 ERROR |
| S6 | Medium | Bug | `Engine/src/ShitEngine/Scene/Scene.cpp:22` + `GameObject.h:29` | `~Scene() = default`、`~GameObject() = default` 均不调 `destroy()/clean()` | `loadSceneFromFile` 解析失败丢弃半构建场景时:PhysicsSystem2D 已自愈注册并建 b2World,但 `destroy()` 不被调 → **b2World 句柄泄漏**;用户在 onDestroy 释放的资源同样泄漏 | `~Scene()` 调 `destroy()`(幂等守卫已有) |
| S7 | Medium | 序列化 | `Engine/include/ShitEngine/Component/CameraComponent.h:50-51` + `Prefab.cpp:69-75` | `fieldToJson/fromJson` 类型覆盖缺口:`SDL_FRect`(16 字节)与 8 字节枚举既不命中命名分支也不命中 `size == 4` 兜底 → **静默跳过** | `CameraComponent::m_viewportRatio` **从不落盘**:编辑器设置的分屏视口比例保存→重载后回退全屏,多相机分屏配置丢失 | 加 SDL_FRect 分支;或对"反射了但不可序列化"的字段输出 WARN |
| S8 | Medium | 序列化 | `Engine/include/ShitEngine/Scene/SceneSerializer.h:45` + `Prefab.cpp:91-102` | 头声明"实例化失败仅跳过对应对象,不抛异常",但 Prefab 路径 `fieldFromJson` 无 try/catch,类型不符即抛(系统字段路径反而有 catch,`SceneSerializer.cpp:78-80`) | `.scene` 中一个字段值类型不符(如 `"m_zoom": "abc"`)→ 异常穿透 → `loadSceneFromFile` 捕获后**整个加载中止**(与头注释矛盾);编辑器 `.prefab` 双击路径异常穿透进 Qt | `applyInternal` 逐字段 try/catch,坏字段 WARN+跳过 |
| S9 | Medium | Bug | `Engine/src/ShitEngine/GameObject/Prefab.cpp:43-47,309-318` | Prefab 不重映射内部 ComponentRef:字段值存捕获时的记录 UUID,实例化只重发组件自身 UUID,不改写字段值 | 编辑器 Ctrl+C/V(同场景):**粘贴出的副本引用解析到源对象的组件**(串线,改错目标);运行时实例内部引用全部解析为 null。对比 Unity:Instantiate 自动重映射实例内部引用 | restoreUuid=false 时按「记录 uuid → 新 uuid」映射表回写引用字段 |
| S10 | Medium | 扫描器 | `Tools/ReflectionScanner/src/Generator.cpp:92-96,199-212` | 无 `SHIT_REFLECT_BODY` 时回退 libclang 数值 offset/size,既无 static_assert(注释明确移除)也无运行时校验;hasReflect=false **无任何告警** | 插件用不同 ABI 工具链(如 MSVC)编译且漏写 friend → 生成时算出的 offset 与编译 ABI 不一致 → 反射读写按错误偏移 → 字段错乱/内存破坏 | fallback 路径生成 `static_assert(offsetof(...) == 扫描值)`;或对 hasReflect=false 输出 WARN |
| S11 | Low | Bug | `Engine/include/ShitEngine/Scene/Scene.h:176` | `componentByUuid` 用 `count()`+`at()` 双重哈希查找 | `ComponentRef::get()` 每帧每引用两次哈希 | `find` 一次 |
| S12 | Low | 性能 | `Engine/src/ShitEngine/Reflection/TypeRegistry.cpp:41-45` | `getType(string_view)` 每次查找构造 `std::string` | `Scene::getSystem(name)`(编辑器每帧)、Prefab 按名实例化等热路径每次堆分配 | 透明哈希异构查找,或调用方缓存 TypeInfo* |
| S13 | Low | 扫描器 | `Tools/ReflectionScanner/src/Scanner.cpp:453-454` | 硬编码 `-target x86_64-w64-mingw32` | 非 Windows 平台扫描语义未验证(CI 四平台);fallback 记录的 typeName/size 按生成时 target 计算,与宿主编译 ABI 可能不一致 | target 由 CMake 检测传入(同 resource-dir 模式) |
| S14 | Low | Bug | `Tools/ReflectionScanner/src/Scanner.cpp:112,149` | findReflectedTypes 不递归进入类体:嵌套 SHIT_REFLECT 类与嵌套 SHIT_ENUM **静默忽略** | `RigidBody2D::Type`、`UIText::TextAnchor` 是嵌套枚举 → 无枚举元数据注册 → 编辑器枚举下拉无命名值;字段仍按 int 往返;叠加 S7 则 8 字节枚举静默丢字段 | 类体也递归(限主文件)或对类内 SHIT_ENUM 输出 WARN |
| S15 | Low | Bug | `Engine/src/ShitEngine/Scene/Scene.cpp:250-252` | UUID 冲突 4 次重试后强制覆盖旧条目 | 持有旧 uuid 的 ComponentRef 此后**解析到新组件(串线)**而非 null(触发概率≈0) | 覆盖时保持 WARN 并考虑拒绝索引 |
| S16 | Low | Bug | `Tools/ReflectionScanner/src/Scanner.cpp:216-221` | `clang_Type_getSizeOf` 负值(CXTypeLayoutError)不设防(size_t 回绕);位域 offset 回退 0 与首字段重叠 | memcpy 读写破坏首成员;size 回绕使按 FieldInfo::size 分配的调用方崩溃(当前无位域成员,待确认) | sizeOf<0 或位域时记 WARN 并跳过 |

**该模块架构观察**(要点):① **字段 JSON 编解码双份且已漂移**——`Prefab.cpp:36-116`(有 unsigned/long/double 分支 + int size 校验)与 `SceneSerializer.cpp:32-81`(无校验、覆盖更窄)各写一份,S3/S7 都是漂移的直接后果,建议抽单一 FieldCodec 供 Prefab/SceneSerializer/编辑器三处复用;② 载体模式未框架化且 readOnly 已三次踩坑(S1 + 两处自我注释),建议 `SHIT_META(SerializeAs="json")` + 统一载体解析注册;③ Prefab 与 SceneSerializer 重复逻辑,版本策略(写不读、缺字段跳过、无类型指纹/迁移)分散两处;④ `Scene.cpp` 533 行职责过载(固定步/系统注册/插件卸载/UUID 索引/延迟增删),建议抽 Scheduler + SystemRegistry;⑤ **ComponentRef 解析依赖"当前上下文活跃场景"**(`ComponentRef.cpp:9-14`)而非 owner 所在场景——多上下文与场景切换延迟窗口内的语义都押在全局单例上,建议走 `owner->getScene()`;⑥ 扫描器与构建系统耦合(硬编码 MinGW target、部分失败不阻断);⑦ 组件挂载热路径每次浅拷贝 m_systems 快照 + 每系统 dynamic_cast。

### 4.4 编辑器动画窗口 / 资源面板 / 导出(模块审计;标 ⚠回归 者为近期 AnimFrame 改造引入)

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| D1 | **Critical** | Bug ⚠回归 | `AnimationClip.cpp:70-86` + `dopesheetwidget.cpp:77` + `animationdock.cpp:228,248,263,426` | **`fromJson` 新格式分支只填 `frameSprites`(清 `frames` 不回填)**;而 Dope Sheet 块模型/播放判定/总时长/缩略图全部只读 `frames`【近期 AnimFrame 改造引入】 | 打开新格式 `.anim` → Dope Sheet 显示「(空)」、无法播放、无双击删除、无缩略图——**编辑器对该格式整体失效**;而编辑器是 `.anim` 的唯一产出途径 → 新格式实际不可用 | 统一真值:fromJson 回填 `frames`,或 Dope Sheet 全读侧改用 `frameSprites` |
| D2 | **Critical** | Bug ⚠回归 | `Editor/animationdock.cpp:377-380` + `AnimationClip.cpp` | **`addSpriteFrames` 对已打开的新格式剪辑只 `frames.push_back` 并 `frameSprites.clear()`**(含本次适配加的 clear)【近期改造引入】 | 打开新格式 `.anim` → 从精灵表拖 1 帧 → 保存:**原动画(跨图集逐帧 rect)被单帧覆盖,不可恢复** | 追加时按帧索引生成 `AnimFrame` 追加进 `frameSprites`,而非清空 |
| D3 | **High** | Bug ⚠回归 | `Editor/dopesheetwidget.cpp:314-336` + `animationdock.cpp:303,380` | **`applyReorder`(拖块排序)直接改 `frames`/`frameDurations`,不同步 `frameSprites`**(widget 持 `m_clip` 指针直改,绕过 dock 的 clear 约定)【近期改造引入:fromJson 展开使 frameSprites 非空,toJson 分支翻转】 | 排序后保存:`toJson` 写 frameSprites 分支 → 落盘仍是**旧顺序**;重开/运行时旧顺序,frameDurations 错配 | 排序/时长操作在单一入口同步双模型(或排序直接改 frameSprites) |
| D4 | **High** | Bug | `Editor/dopesheetwidget.cpp:327` | `ins = (to > from) ? (to - 1) : to;`——`to` 已排除自身(mouseReleaseEvent:284-288),再减 1 属双重校正 | 向右拖动落点提前一格:[A B] 两帧**永远无法互换**;[A B C] 拖中间帧过末帧为 no-op | `ins = std::clamp(to, 0, n-1)` |
| D5 | **High** | Bug | `Editor/animatorgraphview.cpp:250-253` + `mainwindow.cpp:1158` + `animatordock.cpp:296-316` | 拖节点每次 mouseMove 调 `setState`→gen++;mainwindow 每帧 refresh 见 gen 变化即 `rebuildGraph()`→`m_scene->clear()` **销毁拖拽中的图元** | 拖状态节点只生效一帧的位移就卡住,需反复按住才能挪 | 节点移动 release 时批量提交;gen 不因 graphX/Y 变化 bump |
| D6 | **High** | Bug | `Editor/animatorgraphview.cpp:234-334` + `mainwindow.cpp:194-197` | 节点移动从不发 `graphChanged`→`changed`;且 `AnimatorDock::changed` 处理器**只接撤销不 setDirty** | 拖布局后关闭/切场景无保存提示(数据丢失);Ctrl+Z 不能撤销节点移动;**所有状态机编辑均不标脏** | mouseMoveEvent release 补 emit;mainwindow.cpp:194-197 补 `setDirty(true)` |
| D7 | **High** | Bug | `Editor/animatordock.cpp:286,544,554,593` + `mainwindow.cpp:874` | Animator 状态 `assetPath` 三入口一律存**绝对路径**(显示侧才转相对 :502) | 违反「新写入路径统一相对项目根」约定(存储/显示不对称);`.anim` 移目录/换机器后引用全断 | 写入侧用 `AssetPaths::toRelative` |
| D8 | **High** | Bug | `Editor/gameexporter.cpp:247-271` + `Animator.cpp:210-219` | 导出只复制 Assets/ 与起始场景,且只改写起始场景 JSON **顶层**字符串字段;`m_animatorData` 是字符串载体→内部 assetPath 不被遍历改写;Assets/ 外的 `.anim` 不进包 | 导出包在他人机器上所有 Animator 状态 `.anim` 加载失败(仅 WARN,状态不播);本机因绝对路径仍存在而「看起来正常」 | 导出解析 m_animatorData 载体改写 assetPath + 收集 `.anim`/`.prefab` 依赖进包 |
| D9 | **High** | Bug | `Editor/mainwindow.cpp:696-700,1265-1266` + `animationdock.h:50` | `refreshDirtyFromSaved` 只比较场景快照→AnimationDock `changed` 设的 dirty 被抹;退出「保存」只调 `saveScene()` 不存 `.anim`;`markSaved()` 无调用者 | 编辑 `.anim` 后 Ctrl+Z 或保存场景→标题 `*` 消失→关闭无提示→**`.anim` 改动静默丢失**;关闭提示选「保存」后 `.anim` 改动仍被丢弃 | 保存链路同时保存 `.anim`;dirty 判定并入 .anim 状态 |
| D10 | **High** | Bug | `Editor/tilesetdock.cpp:74` | `delete m_gridHost->layout();`——QLayout 析构不删除其管理的 widget,旧 QToolButton 全部残留为孤儿子控件 | 每次切换含 Tilemap 的选中对象,旧的一整格按钮残留并与新网格**重叠显示**、内存持续增长(对比 spritesheetdock.cpp:98 删整个 host 的正确做法) | 先 takeAt 循环 delete 按钮,或删除整个 host 重建 |
| D11 | **High** | Bug | `Editor/spritesheetdock.cpp:169-200` | 帧拖拽手势挂在 dock 自身,但缩略图上的按下被 QToolButton 子控件消费 | 从帧缩略图拖到 Animation 窗口**永远不启动拖拽**——P38 核心交互基本不可用 | 手势下沉到缩略图按钮(子类化/转发) |
| D12 | Medium | Bug | `Editor/spritesheetdock.cpp:102-127` + `tilesetdock.cpp:97-139` | 网格按钮数量无上限(`m_thumbs.resize(rows*cols)` + 逐格建按钮) | 损坏 `.sprite`(rows=cols=1000)→ 100 万按钮 → UI 冻结或 bad_alloc | 上限钳制(如 4096)+ 超限警告 |
| D13 | Medium | Bug | `Editor/dopesheetwidget.cpp:265-295` + `animationdock.cpp:439-451` | 任何块点击 release 时无位移判定,恒走 `applyReorder(from,from)`+`emitChanged`(数据未变);syncTimeline 又清空选中 | 每点一次块标题栏就标脏(假 dirty);点击选中立即被清空 | release 先判位移(< startDragDistance 则只选中) |
| D14 | Medium | Bug | `Editor/animationdock.cpp:193-207` | `save()` 不检查 `f.write` 返回值;也无 `m_clipValid` 守卫 | 磁盘满→文件被 Truncate 成空/半文件但 `m_dirty=false` 且 `saved` 已发;未打开剪辑时保存写出垃圾 `.anim` 并经 reloadAnimatorAsset 写进匹配状态 | 检查写入结果;无剪辑时禁用 |
| D15 | Medium | Bug | `Editor/mainwindow.cpp:888-890` | `reloadAnimatorAsset` 只遍历**当前选中对象**的 Animator | 多个对象引用同一 `.anim`,保存后只有选中对象的动画更新 | 遍历场景全部 Animator |
| D16 | Medium | Bug | `Editor/assetsdock.cpp:134-148,328-334` | `ThumbFileSystemModel::m_thumbs` 按路径缓存永不过期 | 纹理被覆盖后仍显示旧缩略图直到重启 | 提供 `clearThumbs()` |
| D17 | Medium | 性能 | `Editor/assetsdock.cpp:141-142` | `data()`(paint 路径)内 `QImage(path)` 全图同步解码再缩放 | 首次滚过大图目录明显卡顿(UI 线程) | 后台线程生成缩略图 |
| D18 | Medium | Bug | `Editor/gameexporter.cpp:36-55,170-314` | copyDirectory 只增不删;无 staging/中断处理 | 同目录二次导出时已删除资产**残留**;中途失败留损坏半成品包 | 先写临时目录再原子替换 |
| D19 | Medium | Bug | `Editor/gameexporter.cpp:126-129,143-147` | 路径改写中 `QFile::copy` 失败无报告(`&& conflicts` 短路),字段仍被改写 | 包内引用指向不存在的文件,导出日志无痕迹 | 复制失败记入 conflicts 并警告 |
| D20 | Medium | Bug | `Editor/project.cpp:278-286` + `gameexporter.cpp:236-245` | `pluginDllPath()` 只取 `plugins[].front()`;导出 config.json 也只写一个插件 | 多插件项目导出后**其余插件类型运行时缺失** | 遍历全部 plugins |
| D21 | Medium | Bug | `Editor/projectsettingsdialog.cpp:466-497` + `project.cpp:245-253` | `applyToProject` 从零重建 mappings,`setInputMappings` **整体替换**该段 | 手写/未来新增的 `inputMappings` 子键每次「项目设置→OK」后**静默丢失**(读-改-写未保留) | 只更新 actions/axes 子键 |
| D22 | Medium | Bug | `Editor/animatorgraphview.cpp:250-253` + `Animator.cpp:352-358` | 播放态拖节点:`setState` 在播放中 `enterState`→`m_animTime=0` | 播放中拖布局把当前状态动画**每次移动重置到第 0 帧** | graphX/Y 变化不触发 enterState |
| D23 | Medium | Bug | `Editor/spritesheetdock.cpp:55-63` | 文件打开/JSON 解析失败直接 return(无提示) | 损坏 `.sprite` 双击**无任何反馈**,网格仍显示旧精灵表误导 | 失败时提示 |
| D24 | Medium | Bug | `Editor/spriteeditordialog.cpp:142-158` | `m_autoCalc` 恒 true 无关闭途径;且 autoCalc 公式按**双边距**,切帧矩形数学按**单边距** | 手动帧尺寸静默丢失;margin≠0 时推算帧尺寸与实际切帧**错位** | 提供开关;统一 margin 语义 |
| D25 | Low | Bug | `Editor/animationdock.cpp:158-168` + `mainwindow.cpp:984-993` | `openFile` 把「用户取消」与「解析失败」都返回 false | 取消后也弹「无法读取该 .anim 文件」(误导);解析发生在询问之前 | 三态结果 |
| D26 | Low | Bug | `Editor/dopesheetwidget.cpp:85` + `animationdock.cpp:261-270` | 播放头按钳制后的 b.duration 定位,totalDuration 按原始值求和 | 手写 `.anim` 含 0 时长→播放头与块宽不一致 | 统一来源(fromJson 拒绝 ≤0) |
| D27 | Low | Bug | `Editor/projectsettingsdialog.cpp:52-63` + `keys.cpp:7-8` | 按键捕获用 `event->key()`(无修饰);`Key_Shift` 恒映射 LSHIFT | 捕获 Ctrl+S 得 "S";右手 Shift 绑成 Left Shift | 支持修饰组合、区分左右 |
| D28 | Low | Bug | `Editor/projectsettingsdialog.cpp:462` | SDK 目录不校验存在性即写入 | 错误 SDK 路径延后到导出/构建才报错 | OK 时校验 |
| D29 | Low | Bug | `Editor/assetsdock.cpp:477,433-446` | 导入 `QFile::copy` 失败不计不报;重命名不更新 `.sprite` sidecar 与引用 | 导入失败无反馈;重命名纹理后 `.sprite` 仍指旧路径 | 失败计数;重命名联动 sidecar |
| D30 | Low | 待确认 | `Editor/assetsdock.cpp:277-296` | `navigateTo` 依赖模型懒加载,对刚创建目录可能返回无效→直接 return | 新建文件夹后可能不自动进入 | 验证后改用模型加载完成信号 |

**该模块架构观察**(要点):① 无统一资产数据库/导入管线(AssetsDock 直读目录,重命名资产即断链,对比 Unity AssetDatabase GUID);② Dock 间通信全部经 MainWindow 串联(~60 处 connect,引擎已有 EventBus 但编辑器未复用);③ **AnimationClip 双真值无单一维护点**——清空约定散落、widget 直改容器绕过、fromJson 只回填一方(D1/D2/D3 同根);④ 状态机数据以 JSON 字符串载体内嵌,导出路径改写无法遍历其内部(D8);⑤ 撤销栈接入不一致(AnimatorDock 有 undo 无 dirty、AnimationDock 有 dirty 无 undo、图节点移动两者皆无);⑥ 缩略图/预览无异步管线(同步全图解码 + 缓存永不过期);⑦ 导出无清单化校验;⑧ 路径解析已统一但**存储侧不对称**(写入仍存绝对路径,「相对项目根存储」只做了一半)。

### 4.5 编辑器检查器 / 场景树 / 视口(模块审计)

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| I1 | **High** | Bug | `Editor/inspector.cpp:420-425,439-443,1197-1211` + `pathfieldwidget.cpp:124-128` | **名称/Tag/路径字段的每帧回读对「提交仅在 editingFinished」的控件无条件 setText 回写引擎值**(预览 60fps → `onSceneFrameReady` → `refresh()` 每帧);输入未提交期间被旧值覆盖。对照:`animatordock.cpp:501,505` 有 `hasFocus()` 守卫,检查器没有 | **改名/改 Tag/手输路径基本不可用**:每输入一个字符 ~16ms 后被回写为旧值抹掉 | 回读前加 `hasFocus()`/等值守卫(与 animatordock 同模式) |
| I2 | **High** | Bug | `Editor/scenetree.cpp:135-143,180` + `mainwindow.cpp:2124` | 三处程序化选中都不清既有选择(用 `Select\|Rows`,Qt 语义是**合并**;替换需 ClearAndSelect)【待确认运行时行为】 | 视口连续点选→树多选累加;**Del/右键批量删除把之前点过的对象全部删掉**(数据丢失级) | `ClearAndSelect\|Rows`;或 pickSceneAt 先 clearSelection |
| I3 | Medium | Bug | `Editor/inspector.cpp:380` | `m_tabs->setCurrentWidget(m_scroll)`——`m_scroll` 不是 Tab 直接页(页是 `componentPage`),`indexOf` 返回 -1 无操作 | 「选中对象自动切组件页」失效:「系统」页激活时点选对象不切页 | `setCurrentWidget(componentPage)` |
| I4 | Medium | Bug | `Editor/inspector.cpp:787-791,910-916,347-359` | 优先级 SpinBox valueChanged→清签名→下一帧 `rebuildSystemPanel()`→`deleteLater()`(含正在交互的 spin;keyboardTracking 默认 true 每键触发) | 连调优先级一帧后控件被销毁、交互中断、焦点丢失;每次调整全量重建面板 | 优先级变化不整页重建;或编辑中挂起重建 |
| I5 | Medium | Bug | `Editor/viewport.cpp:553-644,922-946` | 全部绘制路径都有 `containsGameObject(m_selected)` 校验,**唯独鼠标/按键路径没有**(mousePressEvent 直接 `m_selected->getComponent<...>()`、frameSelected 直接 `getGlobalBounds()`) | 停止播放到下一帧 sync 的 ≤16ms 窗口内、及 undo/redo 后空场景窗口内,点击视口/F 键 → **对已释放内存调虚函数(UAF)** | 鼠标/按键入口加同款校验(一行闭合) |
| I6 | Medium | Bug+性能 | `mainwindow.cpp:392-398,1175-1192` + `inspector.cpp:367-374` | 编辑名称/Tag/启用→代数 bump→树全量重建 + `setGameObject` **全量重建检查器表单**;setGameObject 不重放 applyFilter | 编辑一个字段后表单整页跳动:焦点丢失、滚动位置重置、过滤态丢失 | 对象级属性走独立提交路径;重建保留滚动/焦点/过滤 |
| I7 | Medium | 性能 | `mainwindow.cpp:2122-2124` + `scenetree.cpp:74-85` | 视口拾取触发 `setGameObject(hit)` **两次**(pickSceneAt 直调 + selectObject 再调);树点击也两次 | 每次点击全量销毁/重建检查器表单 2 遍 | pickSceneAt 去掉直调 |
| I8 | Medium | 架构 | `GameObject.cpp:70-85` + `mainwindow.cpp:1175-1190` | **场景代数被当作通用脏信号**:改名/active/tag 等非结构性变更也 bump | 播放中游戏每帧改 active(闪烁灯)→每帧全量重建场景树+重绑检查器,树闪烁、折叠态丢失 | 区分「结构代数」与「内容代数」,树只响应结构代数 |
| I9 | Low | Bug | `Editor/viewport.h:51` vs `viewport.cpp:600` | 头注释「播放态拾取仍可用」,但左键分支整体被 `m_editEnabled &&` 门控 | 播放态只能靠场景树选对象,与文档矛盾 | 拾取从门控中拆出 |
| I10 | Low | Bug | `Editor/inspector.cpp:604-631` | `findFileDropTarget` 注释「首个命中」,循环实为**最后一个**命中获胜(无 break) | 多个同语义路径字段时拖图填错字段 | 命中后 break |
| I11 | Low | Bug | `Editor/inspector.cpp:1221-1225,1028-1039` | string 字段每键写引擎→每帧回读 `setText(同值)`;若 setText 无等值早退则光标跳行尾【待确认】 | 输入中途光标跳末尾、控件内撤销历史被清 | 等值比较+hasFocus 守卫(与 I1 同批) |
| I12 | Low | Bug | `Editor/viewport.cpp:887-910,618/631` | wheelEvent 不检查 `m_drag`;ColliderBox 拖拽用缓存的 `m_dragPixelScale`,ColliderCircle 在 move 时重查(两处不一致) | 手柄拖拽中滚轮→尺寸/位置跳变 | 拖拽中忽略滚轮;统一重查 |
| I13 | Low | Bug | `Editor/viewport.cpp:211-234` | `m_drawRect` 仅在 paintEvent 更新;setFrame/resize 后、下次 paint 前的鼠标事件用旧矩形;首帧前为空 | 换帧/改尺寸瞬间点击落点偏差 | letterbox 几何提取为 `updateDrawRect()` 同步计算 |
| I14 | Low | Bug | `viewport.cpp:249-250` + `CameraComponent.cpp:57-87` | `worldToScreen` 返回**相机 viewport 局部**坐标,Viewport 按整帧逻辑坐标映射;scene_camera 默认 ratio {0,0,1,1} 时巧合重合【潜伏】 | ratio 一旦变为子区域,Gizmo/拾取整体错位压缩到左上 | 按 viewportRatio 换算回整帧,或断言全视口 |
| I15 | Low | 性能 | `Editor/scenetreemodel.cpp:25-55,209-226` | index/rowCount/parent 每次现场构建 std::vector,模型无脏缓存 | 树布局/过滤期间每节点重复分配,被 I8 放大 | 缓存层级+代数失效标记 |
| I16 | Low | Bug | `Editor/scenetreemodel.cpp:157-182,119-137` | dropMimeData 的 `row` Q_UNUSED→同父兄弟重排被拒;mimeData 仅取 `indexes.first()` | 只能改父子不能重排兄弟;多选拖拽只移动第一个 | 按 row 插入;或明确禁用多选拖拽 |

**该模块架构观察**(要点):① 检查器逐帧全量回读+重建(对照 Unity SerializedObject 改动驱动 / Godot property_changed 增量),建议改「脏标记+等值增量 setText」,表单重建只在结构变化发生;② **面板与引擎反射强耦合**:各分支各自 `reinterpret_cast<T*>(GetFieldPtr)`,只有 int 分支校验 size——当前所有反射枚举都是 `: int` 所以安全,未来 `: uint8_t` 枚举会被按 4 字节写坏相邻内存(与 S3 同根,统一 FieldCodec 修复);③ **选中态单一真源缺失**:分散在 5 处、靠 ≥5 条相互打补丁的路径同步,4 处注释都在解释各自的坑——是 I2/I5/I7 的根因,建议 SelectionManager 单一真源+面板订阅;④ Viewport 职责过载(991 行 12 种 DragMode,像素换算至少 4 处各写一遍),建议拆 Gizmo/Handle overlay 与坐标换算工具;⑤ 场景代数当通用脏信号(建议双代数);⑥ 已验证无问题:m_readbacks/m_systemReadbacks 分离、拖拽环检测双保险、删除快照收集、F 键聚焦除零钳制、播放态墓碑清扫联动。

### 4.6 核心 / 宿主 / 输入(审计人亲自核查)

**结论:扎实,无显著缺陷。** 已逐文件核验:
- `Time.cpp`:混合等待(sleep+忙等,规避 Windows 15.6ms 定时器精度)、dt 钳制 100ms、`SetTargetFPS(0)` 不限帧率有守卫——正确。
- `EventBus.cpp`:锁内 swap + listener 快照,回调内 Subscribe/Unsubscribe/Emit 安全——正确。
- `Input.cpp`(355 行):三态语义正确(Down=current&&!prev);KEY_DOWN 带 `!repeat` 守卫;动作/轴编译与求值、注入坐标优先 + letterbox 转换、键名别名解析——全部扎实。
- `Runtime/src/main.cpp`:**销毁顺序正确**(`Game::Destroy()` 先于 `pluginManager.UnloadAll()`,注释明言防组件析构 UAF);空场景兜底走统一 LoadSceneFromFile。
- `PluginManager.h`:`kAbiVersion` 校验、C ABI 入口、UnloadAll 契约文档化。

| # | 严重度 | 类别 | 位置 | 问题 | 后果/复现场景 | 建议修复 |
|---|--------|------|------|------|--------------|----------|
| C1 | Low | 文档 | `AGENTS.md`(关键约定节) + `CHANGELOG.md:330` | AGENTS.md 声称"帧率控制通过 Time::SetTargetFPS() + **SDL_AddTimer** 实现"——代码中**无任何 SDL_AddTimer 调用**(已 grep,仅 CHANGELOG 历史记录提及),实际机制是 update() 内联混合等待 | 按文档找定时器/改帧率机制会扑空 | AGENTS.md 更新为"内联混合等待(sleep+忙等)" |
| C2 | Low | 架构 | `Engine/include/ShitEngine/Plugin/PluginManager.h:53` | `GetLastLoadError()` 是 static **跨实例共享状态**(两个上下文互相覆盖错误信息);AGENTS.md 只声明 Log 一个例外 | 编辑器双上下文下插件加载错误信息互相覆盖 | 移入实例成员或按上下文存放 |
| C3 | Low | 性能 | `Engine/src/ShitEngine/Core/Time.cpp:50-52` | 忙等自旋每帧最多占 ~1.5ms 的整核(60fps 下约 9% CPU) | 笔记本/低功耗设备风扇转、耗电 | 可选:`SDL_DelayNS` 精度提升(sleep granularity)或条件编译降级 |

**注**:`Time` 的 dt 钳制(100ms)与 `Scene` 固定步夹取(3 步)构成**双层防爆炸**,卡顿后进入慢动作而非穿透——设计正确。

---

## 5. 参考 Unity / Godot 的改进路线图

### 5.0 立即修复清单(按杠杆排序,与 Phase 0 衔接)

**A. 一行~几行的守卫类修复(半天内可全部完成,先做)**

| 优先 | 缺陷 | 修复 | 工作量 |
|------|------|------|--------|
| 1 | **S1**(Critical)`Tilemap::m_gridData` 标了 readOnly,每次保存丢全部瓦片 | 去掉 `.readOnly = true` | 1 行 |
| 2 | **D1+D2**(Critical,⚠回归)新格式 `.anim` 编辑器全盲 / 拖帧清空原动画 | 短期:`fromJson` 同时回填 `frames`(从 frameSprites 无法反推网格索引时按序号 0..n-1 填充仅供 Dope Sheet 计数);根治:见 §5.0.B | ~20 行 / 根治见 B |
| 3 | **M2**(High)插件卸载不清理注册系统 → UAF | `loadProjectConfig`/`unloadPlugins` 复用 `reloadProjectPlugins:194-204` 的 2.5 段 | ~12 行 ×2 |
| 4 | **I1**(High)检查器名称/Tag/路径每帧 setText 抹掉输入 | 回读前加 `hasFocus()`/等值守卫(照抄 `animatordock.cpp:501,505` 同款) | ~6 处 |
| 5 | **I2**(High)程序化选中用 `Select` 不清旧选 → 批量删除误删 | 三处改 `ClearAndSelect\|Rows` | 3 行 |
| 6 | **M1**(High)播放中 Ctrl+S 保存运行态场景 | `saveScene` 开头加 `if (isPlaying()) setPlaying(false);` | 2 行 |
| 7 | **M3**(High)损坏 `.prefab` 异常穿透 → 编辑器 terminate | `instantiatePrefab` 包 try/catch + 回滚(对齐 `openScenePath:626` 同款);`fieldFromJson` 加类型校验 | ~10 行 |
| 8 | **I5**(Medium)viewport 鼠标/按键路径缺 `containsGameObject` 校验 → UAF 窗口 | 与绘制路径同款校验 | ~4 处 |
| 9 | **E3**(Medium)Tilemap tileId 无上界 → 每帧错误刷屏 | 渲染前钳制到纹理容量 | 2 行 |
| 10 | **M4+M5**(Medium)输入转发粘键(KeyRelease 被 Ctrl 丢弃 + 失焦不补发 KEY_UP) | Release 不含 Ctrl 键值时照发;FocusOut/WindowDeactivate 补发已按下键的 KEY_UP | ~15 行 |

**B. 结构性修复(按依赖排序)**

1. **D3 + D1/D2 根治 + E7**:编辑器动画编辑统一到 `frameSprites` 单一真值(Dope Sheet 块/播放判定/总时长/缩略图/追加/排序全部读写 frameSprites,`frames` 降级为只读旧格式入口);顺手统一 `stable_sort`。这是原计划 Phase 2 的编辑器部分,也是本次 AnimFrame 改造的收尾。
2. **S3 + S7 + I2 的反射写路径**:抽**单一 FieldCodec**(类型分支 + size 校验 + try/catch 逐字段)供 `Prefab::fieldToJson/fromJson`、`SceneSerializer::deserializeFieldValue`、检查器三处复用——一次消灭 S3(堆破坏)、S7(SDL_FRect/8 字节枚举静默不落盘)、S8(异常穿透)三个同根缺陷。
3. **S4 + S5(扫描器/注册表)**:部分解析失败非零退出;TypeRegistry 命名空间限定键或同名告警。
4. **§1 全部 12 条**(CI 盲区/stamp 入库/SDK 命名/零测试/文档指引等)——见 §1 表,多数半天内可完成。
5. **D5+D6+D22(Animator 图节点链路)**:release 批量提交 + gen 只在 release 后变化 + `changed→setDirty` 接线。
6. **D9+D14(保存/dirty 一致性)**:保存链路同时存 `.anim`;dirty 判定并入 .anim 状态;`save()` 检查写入结果。
7. **M6+M7+M12(QSettings/模态/pending)**:统一 SettingsService 键名;模态对话框暂停引擎;prefab 落点覆盖 pending。

### 5.1 五个需要作者拍板的架构决策点

| # | 决策 | 选项 | 建议 |
|---|------|------|------|
| D1 | **渲染后端** | 继续 SDL_Renderer(简单、够用、无 Shader) / 抽象接口后接 SDL_GPU 或 bgfx | **先抽接口不换后端**:零风险拿到解耦收益,等真需要 Shader 时再落地 |
| D2 | **脚本方案** | 原生 DLL(C++ ABI 强耦合,改引擎即重编插件) / 轻量脚本 VM(Lua/Wren) / Godot 式稳定 C ABI | **中期下沉玩法逻辑到脚本 VM**,引擎能力经稳定 C ABI 暴露;原生插件只留给高性能扩展。这直接消灭 2.5 的脆弱性 |
| D3 | **资产引用** | 路径(现状) / GUID | **GUID**(Unity `.meta` / Godot UID 路线),否则 2.2 无解 |
| D4 | **编辑器同步** | 每帧全量回读(现状) / 属性变更通知 + 脏标记 | **脏标记/版本号驱动**,顺带把撤销从整场景快照改为动作级 |
| D5 | **测试门槛** | 继续人工验证 / 引入测试 + CI 门禁 | **必须引入**,否则每次重构都是赌博 |

### 5.2 Phase 0 · 稳定性与工程门禁(1–2 周,最高杠杆)

1. **修 CI 盲区**:加一个 job 以 `-DBUILD_TOOLS=ON` 配置并构建,然后 `git diff --exit-code Engine/generated` —— 反射代码一旦与头文件不一致,CI 立刻红。
2. **从 Git 移除 stamp 文件**(`.reflect-engine-stamp`/`.reflect-stamp`)并加 `.gitignore` 规则。
3. **引入测试框架**(doctest 或 Catch2,头文件库,零依赖负担)+ `ctest` + CI test job。首批用例(全部是本次审计暴露过风险的区域):
   - 反射 `FieldInfo` offset/size 与 `sizeof/offsetof` 一致性
   - `.scene` / `.prefab` / `.anim` 序列化往返(含 ComponentRef UUID、旧格式兼容)
   - 固定步循环语义(60Hz、单帧最多 3 步、暂停、接触事件派发时机)
   - `AnimationClip` 新旧格式互转(本次改动刚建立的兼容层)
   - `ResourceManager` 懒加载/失败重试/资产根回退
4. **修 SDK 打包**:`ShitEngineConfig.cmake.in` 按实际产物命名(`ShitEngine.dll` / `libShitEngine.dll.a`);install 前清理目标前缀;发布前校验包内 DLL 列表与时间戳。
5. **修文档与警告**:三处 `run-reflectionscanner` 指引改为"`-DBUILD_TOOLS=ON` 重新配置";`Editor/CMakeLists.txt` 最低版本升到 3.20、`project` 版本对齐。
6. **补最小剖析设施**:引擎侧统计每系统 `update`/`fixedUpdate` 耗时、绘制批次数、物理步耗时;编辑器加覆盖显示。**这是后续所有性能工作的地基,也只需 ~200 行。**
7. 修复第 4 节的全部 **Critical / High**。

### 5.3 Phase 1 · 序列化与资产管线(1–2 月,消除架构病根)

8. **反射层容器支持**(2.1):这是本阶段核心。加容器描述符后:退役全部"字符串载体"、合并 `AnimationClip` 双模型、让检查器原生编辑数组/枚举/引用。
9. **序列化版本化**(2.3):`fromJson` 校验文件 `version`,给出"文件版本高于引擎"的明确错误;每个组件提供可选 `migrate(fromVersion)` 钩子。
10. **资产数据库**(2.2 / D3):GUID + `.meta` 侧车(GUID、导入设置、类型)、依赖图、移动/重命名自动改写引用、缺失资产在编辑器里显示为占位对象而非空指针。
11. **Prefab 增强**:变体(variant)、嵌套 prefab、属性覆盖记录(对照 Unity Prefab Overrides)。
12. **热重载**:文件监听 → `Resource::load()` 原地重载(资源层已预留此设计口),`.anim`/纹理/`.scene` 改完立即生效;这是编辑器体验质变点。

### 5.4 Phase 2 · 玩法能力补齐(2–4 月)

13. **物理**:碰撞层/掩码(高)、传感器语义、射线与形状查询(高)、关节补充与调试绘制完善。
14. **动画**:动画事件(中高,配合音效/特效触发)、BlendTree/BlendSpace、动画层与遮罩。
15. **Tilemap**:自动地形(RuleTile 式)、瓦片碰撞体生成、多层与图层偏移。
16. **2D 粒子系统**:发射器 + 生命周期 + 曲线插值 + 渲染排序接入。
17. **相机**:跟随/平滑/边界约束/多目标(对照 Cinemachine 的简化子集)。
18. **UI**:布局容器(Row/Column/Grid)、ScrollView、Hover/Drag/Focus 导航、样式表。
19. **音频**:总线效果链、流式播放、声道池化。
20. **运行时存档**:场景状态 + 用户数据的版本化存档。
21. **编辑器生产化**:多选批量编辑、Prefab 笔刷、Tilemap 笔刷增强、构建管线(场景清单/资源打包/体积报告)、属性搜索与收藏。

### 5.5 Phase 3 · 架构升级(4–8 月,依 D1/D2 决策)

22. **渲染后端抽象**(2.4 / D1):`IRendererBackend` + 可选 GPU 后端 → Shader/后处理/批处理统计。
23. **脚本 VM + 稳定 C ABI**(2.5 / D2):玩法逻辑下沉,插件可以跨引擎小版本;配套导出 API 文档化与版本协商。
24. **资源异步加载 + 作业系统**(2.10):后台解码纹理/音频,主线程不卡帧。
25. **场景流式/附加加载**(2.11):常驻场景 + 关卡场景分层,异步切换。
26. **性能基线化**:帧预算、分配审计、批处理统计纳入 CI 报告(与 Phase 0 的剖析设施衔接)。

### 5.6 可从 Unity / Godot 直接借用的"低垂果实"

| 机制 | 出处 | 为何值得抄 |
|------|------|-----------|
| Layer Collision Matrix(矩阵式碰撞配置) | Unity | 比在代码里逐对判断优雅得多,UI 也直观 |
| Prefab Overrides 可视化(改了哪里一目了然) | Unity | 直接解决"实例与源资产不一致"的困惑 |
| `_get_property_list` 式属性暴露 | Godot | 比纯宏标注更灵活,可运行时动态暴露属性 |
| `@tool` 脚本(编辑器内直接执行脚本) | Godot | 编辑器扩展成本骤降 |
| Scene 实例化(场景即可复用组件) | Godot | 比 Prefab 更通用的组合单元 |
| AnimationEvents / method track | Unity / Godot | 动画驱动音效与特效的最短路径 |
| UndoRedo 动作式撤销 | Godot | 替代整场景快照,性能与可合并性双赢 |
| 编辑器内"播放中改参数" | 两者皆有 | 调参效率的关键:本项目已有播放态,值得补 |

---

## 6. 附录:证据与复现方式

| 结论 | 复现方式 |
|------|----------|
| 1.1 SDK 命名不一致 | 读 `Engine/CMakeLists.txt:90-97` 与 `Engine/cmake/ShitEngineConfig.cmake.in:15-19,99-104`;`ls out/build/x64-debug/bin` 与 `ls out/sdk-msvc/bin` |
| 1.2 CI 不校验生成代码 | 读 `.github/workflows/build.yml` 四处 `-DBUILD_TOOLS=OFF`;`git ls-files Engine/generated/reflection` 得 32 项 |
| 1.3 stamp 入库 | `git ls-files Engine/generated/reflection \| Select-String stamp` |
| 1.5 无效命令指引 | `Select-String -Path CMakeLists.txt,AGENTS.md,Editor/ROADMAP.md -Pattern 'target run-reflectionscanner'` 得 6 处;目标创建在 `CMakeLists.txt:66-84` 的 `if(BUILD_TOOLS)` 内 |
| 1.7 零测试 | `Get-ChildItem -Recurse -Include *test*` 无源码命中;CI 无 `ctest` |
| 2.1 反射不支持容器 | `Engine/include/ShitEngine/Reflection/TypeInfo.h` 的 `FieldInfo` 仅含 offset/size;三处载体:`Tilemap::m_gridData`、`Animator::m_animatorData`、`AnimationComponent::m_clipsData` |
| 2.3 无版本校验 | `Engine/src/ShitEngine/Scene/SceneSerializer.cpp:270-297`(fromJson 只检查 `objects`) |
| 2.4 渲染后端 | `Engine/src/ShitEngine/Render/Renderer.cpp:20,119,161,168` |
| 2.6 静态例外 | `Engine/include/ShitEngine/Plugin/PluginManager.h:53` |
| 3 功能缺口 | 对 `Engine/include` 全量 grep `Particle\|Shader\|LayerMask\|Raycast\|Tween\|Atlas\|Localization`,除 UI 文档注释提及 "Raycasting" 外零命中 |
| 引擎可编译性 | 本次审计期间以 `-DBUILD_TOOLS=ON` 全量重编 `ShitEngine.dll` 成功(932 步,`[931/932] Linking`) |

---

*报告生成:全源码审计(7 路模块审计 + 审计人构建/CI/引擎核心横向核查)。所有 Critical 与标注【已复核】的条目均经审计人亲自验证;其余条目附带验证方法或「待确认」标注。*
