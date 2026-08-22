# Changelog

本项目的所有重要变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 新增

- **Resource 基类 + 统一资源缓存（引擎重构）**：`Shit::Resource` 基类（路径/加载状态/错误信息/会话级 uuid）+ `Texture`/`Font`/`Audio` 子类封装 `SDL_Texture`/`TTF_Font`/`MIX_Audio`；`ResourceCache<Key,Res>` 模板统一「查缓存→加载→插入→回收」——原 TextureManager/FontManager/AudioManager 三份 ~150 行复制粘贴折叠为 ResourceManager 内三个缓存（门面 `GetTexture/GetFont/GetAudio` 签名不变，调用方零改动）；顺手修正 Font/Audio 把缓存未命中误打 ERROR 的日志级别；新增 `GetTextureAsset/GetFontAsset/GetAudioAsset`（访问状态/错误/uuid）与 `GetResourceByUuid`（会话内查询，为将来热重载留口；持久化资产 uuid 注册表按决策暂不做）。所有权：缓存 unique_ptr 独占、调用方借用裸句柄，集中销毁顺序保持（将来热重载走原地 reload 换内部句柄）
- **资产根解析（引擎）**：`ResourceManager::SetAssetRoot` + `ResolveAssetPath`——相对路径先按资产根解析、未命中回退进程 cwd（Runtime `resource/...` 场景零影响）；编辑器打开项目时资产根=项目根，项目内相对路径资产从此可直接加载
- **资源路径字段统一体验（编辑器）**：新 `PathFieldWidget` 复合控件（`[路径框][✕][…浏览]`）——**字段级拖放**（按扩展名过滤，拖到哪个字段填哪个）、**浏览选择**（QFileDialog 按字段语义过滤：图片/音频/字体/.anim/.scene/.prefab）、手输相对/绝对统一解析；字段语义按字段名关键字推导（texture/sprite/sheet/tileset/icon/audio/sound/music/font/anim/clip/scene/prefab），覆盖 SpriteRenderer/Tilemap/UIText(Input)/AudioSource 等全部路径字段，一次一提交接撤销栈+dirty；**面板级拖拽兜底**（P31 恢复——曾实现后被反射重写提交误删，从 git 历史找回并适配 Tab 结构：拖到面板空白按扩展名匹配，未知扩展+唯一 string 字段兜底）
- **统一路径服务 `Editor/assetpaths.h`**：`toRelative/toAbsolute` 一对函数（项目根注入），替换 Animator/Animation/SpriteSheet/Tileset 四份分歧实现（TilesetDock 补项目根感知）；**新写入路径统一相对项目根存储**（逃逸/无项目落绝对；旧场景绝对路径双向兼容，导出器本就兼容两种格式）
- **P34 回归修复**：场景树 Ctrl+F 名称过滤框 + 检查器组件/字段搜索框（被 ffa1a5a 误删，恢复并适配 Inspector Tab 布局——搜索框挂组件页顶部、按组件行区间整块显隐）
- **编辑器便捷操作 7 项**：**F 键聚焦**选中对象（视口帧化：编辑相机对准对象+包围盒调 zoom）；**Ctrl+D 原地复制**对象（复用内部剪贴板粘贴路径，重名去重+同父+选中副本）；**状态栏常驻信息**（选中对象名 + 鼠标世界坐标，每帧刷新）；**播放/停止自动切换视口 Tab**（▶ 自动切运行视口、■ 切回场景视口）；**滚轮缩放锚定鼠标光标**（原以相机中心缩放，光标处物体会漂）；**Ctrl+P 暂停/继续**、**Ctrl+Shift+P 单步**（暂停态推进一帧，`EnginePreview::singleStep`）

- **属性面板 Tab 化 + 对象属性编辑 + 资源类型图标（编辑器 + 引擎，参考 Unity）**：
  - **Inspector 双页 Tab**：「组件」（对象属性 + 组件字段 + Add Component）/「系统」（场景系统列表 + 系统属性）两页常驻，选中对象自动切组件页；播放态两页只读但仍可切换查看运行时值；系统页回读与组件页回读分开管理（重建系统页不再清掉组件页回读），系统列表签名检测不再依赖未选中态（选中对象时物理自愈注册等变化也能刷新）
  - **GameObject 对象属性**：组件页顶部 Unity 式 `[启用 ✓][名称]` 行 + Tag 编辑行（均入撤销栈、每帧回读、随场景保存）；场景树失活对象灰显（含父链级联态）
  - **引擎 active 语义**：`GameObject::isActive()`（自身）/ `isActiveInHierarchy()`（父链级联，Unity 语义：父失活子随失活）/ `setActive()`；失活对象不渲染（RenderSystem）、不更新行为（BehaviorSystem，重新激活后 onStart 补跑）、UI 不参与绘制/射线/按钮（UIRenderSystem 一处过滤三处生效）、物理刚体移出模拟（`b2Body_Enable/Disable`，仅状态变化时调用）；失活对象上的相机不计入场景相机兜底判定
  - **tag 落盘**：对象级 `tag`/`active` 字段随 `.scene` 序列化（空标签/启用态不写入保持旧文件兼容，旧文件回退空 tag + 启用）
  - **资源面板文件类型图标**：`.scene` 蓝 SC / `.anim` 橙 ▶ / `.prefab` 青 PB / `.sprite` 紫 SP / `.wav` 绿 ♪ / `.ttf` 深灰 A 手绘彩色圆角图标（16/32/56 三档，树与网格共享同一模型一处生效）；私有后缀此前全落 OS 默认白板文件图标

- **精灵表功能（P38，编辑器）**：Unity 式图片精灵表定义与动画帧编辑——资源面板右键图片「定义为精灵表…」弹出网格配置对话框（行列/帧宽高/边距/间距，左侧纹理预览叠加网格线，自动推算），保存 `.sprite` 元数据文件；`.sprite` 双击打开精灵表视图 Dock（缩略图网格，点选帧后拖拽 → 自定义 MIME `application/x-sprite-frame`）；Animation 窗口接受精灵拖入，自动新建/追加帧到当前剪辑
- **编辑器易用性批次（P32~P36，编辑器 + 引擎）**：
  - **部署与日志（P32）**：`cmake --install` 后 SDK/安装目录自带 Qt DLL（windeployqt），本地双击 `Editor.exe` 即可运行；引擎文件日志可切换到任意目录，编辑器打开项目后日志写入项目 `.shitengine/log`（不再落在启动目录）；日志面板新增「保存日志…/清除」按钮（编辑器侧日志此前仅内存、崩溃即丢）
  - **失败反馈可感知（P33）**：构建失败弹窗显示真实原因（含 cmake 未安装在 PATH 的提示）；插件加载失败（DLL 缺失/ABI 不匹配）与热重载失败弹窗提示；导出前校验 SDK 运行库与脚本 DLL 完整性（缺则导出失败，不再产生"导出成功"的残缺包）；启动恢复上次项目失败弹窗说明；`.anim` 资产读取/解析失败弹窗提示
  - **查找导航（P34）**：场景树顶部名称过滤框（Ctrl+F 聚焦，匹配节点保留祖先链）；检查器组件/字段搜索框（按类型名/字段名/显示名过滤，清空恢复）
  - **资源管理（P35）**：资源面板树/网格右键菜单——刷新 / 新建文件夹 / 导入文件… / 重命名 / 删除（删除前确认）
  - **批量与入口（P36）**：场景树多选（Ctrl/Shift）时检查器进入批量编辑——公共组件字段编辑应用到全部选中对象（一次撤销；引用字段/只读字段不提供批量编辑）；命令行入口 `Editor.exe <xxx.scene>` / `--project <dir>`（支持 .scene 文件关联）
- **文件日志落盘（引擎）**：日志除控制台外同时写入当前项目 `.shitengine/log/log_YYYYMMDD_HHMMSS.txt`（按启动时间归档），每条日志即时 flush——进程崩溃时最后一段日志不再丢失，方便排查闪退
- **文件拖到属性面板自动填充（P31，编辑器）**：从资源面板或文件管理器把文件拖到检查器，按扩展名语义自动匹配选中对象的路径类 `std::string` 字段并填充（图片→texture/sprite/sheet，音频→audio/sound，字体→font，`.anim`→anim/clip，`.scene`/`.prefab`→scene/prefab；扩展名不在已知表且对象只有一个路径字段时兜底填充）；写入接入既有 dirty + 撤销栈，拖拽悬停虚线高亮提示，播放态只读锁拒绝

### 修复

- **选中唯一对象后属性面板卡在「系统属性」（高，编辑器）**：视口点空白取消选中时只清了检查器/瓦片/动画面板、不清场景树 current——再点树上仍是 current 的那行时 Qt `setCurrentIndex` 相等早退不发 `currentChanged`，`objectSelected` 永不发出，检查器卡死在未选中的系统面板，直到新建对象把 current 挪走才恢复。点空白现统一清空树选中 + Gizmo（新增 `SceneTree::clearSelection`，current 置无效经 `objectSelected(nullptr)` 走统一清空路径）；场景树补 `clicked` 兜底重发（点击已是 current 的行也必发 `objectSelected`，任何选中态失同步可点击自愈）；`newScene`/`openScenePath` 清场前先清检查器/视口选中态（对齐 `rollbackScene`，消除对象销毁到下一帧同步之间的悬垂指针窗口）
- **全库 BUG 批量修复（反射 / 序列化 / 引擎核心 / 编辑器，14 项）**：
  - **反射·属性注解挂错声明（严重）**：`SHIT_PROPERTY(isFlipped, setFlipped)` 与 getter 之间隔了注释，GNU 属性前向附着到 `onAfterDeserialize` 上，属性类型被扫成 `void`——此前靠生成器按属性名白名单硬猜才碰巧正确。注解已紧贴 getter；Scanner 改为按 getter 名查类内方法表解析真实返回类型（注解放错位置时兜底），类型为 void/引用时 WARN 并跳过（不再生成不可编译代码）；`SHIT_METHOD` 补齐 `paramTypes` 收集（此前恒空，带参方法一生成就编译失败），`TypeInfoBuilder::Method` 增加参数类型列表；字段/方法/属性三路 SHIT_META 提取统一为"仅 `({...})` 结构化语法"过滤（简单标记误入会拼出未声明标识符）
  - **属性不序列化（高）**：检查器可编辑的反射属性（SpriteRenderer 的 sourceRect X/Y/W/H、flipped）保存场景后丢失——`Prefab::Capture` 只遍历 fields。现已随 `properties` 段落盘并经 setter 恢复（撤销快照/复制粘贴同享）；`setSourceRectX/Y/W/H` 物化 nullopt 时按纹理尺寸兜底（此前 `{0,0,0,0}` 会让精灵 0×0 消失且无法回到整图），W/H getter 整图态返回实际纹理尺寸
  - **退出时插件卸载顺序颠倒（严重·UAF）**：Runtime 与编辑器预览 `stop()` 均先 `UnloadAll()` 再 `Game::Destroy()`——场景内插件 Behavior 组件析构的虚调用打到已解除映射的 DLL vtable（退出必崩）。已统一为"先销毁引擎再卸载插件"（与热重载路径同序）
  - **运行态切场景重复注入相机（高）**：`ensureDefaultCamera` 只扫 `m_gameObjects`，运行中加载场景时对象全在 pending 队列 → 文件里有相机也再补一个（双相机渲染两遍）。新增 `Scene::hasEnabledCamera()/findGameObjectByName()` 覆盖正式 + pending 容器
  - **SceneSerializer 系统 Color 字段越界读写（中高）**：`Color`（4×uint8_t）被按 `glm::vec4`（16 字节）读写——反序列化越界写 12 字节破坏相邻内存；int 兜底分支补 `size == 4` 校验（防 libclang 模板退化 "int" 按真实对象宽度误写）
  - **场景文件损坏时父子关系错位（中）**：`fromJson` 遇非法对象条目跳过不占位，后续所有对象的 `parent` 下标整体错位一格；非法条目现以 nullptr 占位并在层级重建时跳过
  - **`setScene` 级联迭代器失效隐患（中）**：子物体级联的 `onAttach` 可重入改 `m_children`；改为快照遍历 + 归属校验（AGENTS.md 约定）
  - **`Game::destroy()` 不复位暂停态（低）**：Destroy → 重新 Init 后引擎静默保持冻结
  - **Animation 窗口无法加帧（严重·编辑器精简过度删除）**：接收精灵帧拖入的 `dropEvent/addSpriteFrames` 与左侧面板一起被删，且无替代入口——新建剪辑后帧既加不进也删不掉。已恢复拖入接收端（缩略图按 .sprite 元数据 cols 切帧）+ 实现"双击时间轴块移除帧"（提示文案早已承诺）；窗口新增 `setProjectRoot` 解析项目相对纹理路径
  - **瓦片面板信号孤儿化（严重）**：视口瓦片画笔（`setPaintTileId/paintTileAt/网格线/PaintTiles 拖拽`）被删而 TilesetDock 原样保留——点选瓦片/橡皮完全无效。已恢复视口画笔与 `tileSelected → setPaintTileId` 接线
  - **拖精灵帧行列错位（高）**：落点精灵的列数按纹理宽度反推，与 .sprite 元数据 cols 不一致时拖 A 帧显示 B 帧；信号补传 `cols`（与精灵表缩略图、引擎 `getFrameRect` 同源），落点同时由"视口中心"改为真实鼠标位置
  - **只读属性行不刷新（中）**：回读 lambda 调 getter 后丢弃结果，标签冻结在构建时刻；现每帧刷新
  - **属性/字段编辑不入撤销栈（中）**：`valueChanged` 先写值后 `emit fieldEdited`——undo 的 before 快照已含首次改动，单步编辑 `before == after` 被丢弃；全部（属性 4 分支 + 字段 7 分支 + 系统面板 8 分支）改为先 begin 再写
- **项目插件 DLL 加载失败 126（引擎命名，高）**：插件按模块名静态导入引擎 DLL，而 Debug 构建的引擎带 `-d` 后缀（`ShitEngine-d.dll`）、MinGW 构建带 `lib` 前缀——用 Debug 编辑器加载 SDK 构建的插件时名字对不上，Windows 找不到 `ShitEngine.dll` 模块报 126（往项目 bin/ 拷引擎 DLL 无效：默认搜索路径不含插件所在目录，且重复加载会分裂引擎静态状态）。引擎 DLL 现已全配置统一命名 `ShitEngine.dll`，插件导入名恒与宿主进程内引擎模块一致，加载器直接复用内存实例；`PluginManager` 同时改用 `LoadLibraryExW`（`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS`，支持解析插件自带依赖、宽字符路径）并在 126 错误信息中给出原因提示
- **`forEachComponent` 迭代器失效（高）**：`onAttach`/`onStart` 等回调内 `removeComponent` 修改组件 map 会使实时遍历的迭代器失效（`System::init` 补扫等路径）；改为快照遍历 + 回调前归属重验
- **`createGameObject` 挂在场景时序不一致（中）**：与已修复的 `addGameObject` 不同，`createGameObject` 先 `setScene` 后入容器，`onAttach` 触发时对象尚不在 `m_gameObjects`，运行态下组件挂载时 `m_scene` 为 null 不注册不索引；已统一为「先入容器再 setScene」
- **`processPendingRemoveSystems` 悬垂引用（低）**：`destroy()` 回调内 `unregisterSystem` 追加条目使 vector 重分配后，循环持有的 `const auto&` 引用悬垂；改为按值拷贝
- **`m_viewportRatio` 反射类型名错误**：libclang 解析退化 `int`，修正为 `SDL_FRect`

---

## [1.4.1] - 2026-08-16

### 修复

- **编辑器 Gizmo 拖拽不更新 Box2D 刚体位置**：移动/旋转带 `RigidBody2D` 的对象时，只改了 `TransformComponent`，没有同步 Box2D 刚体——`Static` 刚体（如地面）的碰撞箱显示与实际物理碰撞位置永久错位；已修复为 Gizmo 拖拽后同步调 `RigidBody2D::setTransform`

---

## [1.4.0] - 2026-08-14

> 🎉 **编辑器驱动 + 资产化工作流**。从 v1.3.0 的"引擎可运行"升级到"编辑器做所有事"：场景系统可视化、动画资产化、场景管理统一化、跨平台 CI 构建。

### 新增

- **场景系统面板（P30，引擎 + 编辑器）**：Unity 风格——检查器未选中对象时显示「场景系统」面板，可添加/移除系统（含插件自定义系统，反射收集），编辑系统优先级，修改系统反射字段（如 `PhysicsSystem2D` 的 `Gravity`、`Pixels Per Meter`，即时生效）：
  - 引擎侧：`System` 基类加入 `SHIT_REFLECT(WhiteList)` 反射链（`onFieldChanged` 虚函数）；`PhysicsSystem2D` 暴露 `m_gravity`/`m_pixelsPerMeter` 反射字段；`Scene` 新增按类型名注册/查询/移除/枚举系统 API
  - `.scene` 序列化：`"systems": [{"type": "PhysicsSystem2D", "fields": {...}}]` 数组落盘，排除默认三系统（BehaviorSystem/RenderSystem/UIRenderSystem），旧文件无此字段向后兼容
  - 热重载保护：卸载 DLL 前 `flushPendingSystemRemovals` 清理插件系统，防止 vtable 悬垂
- **场景管理统一化**：`Scene` 标记 `final`（不再支持继承），`LoadSceneFromFile` 为加载场景的唯一公开入口；Runtime 空场景兜底改用临时文件 + `LoadSceneFromFile`，编辑器预览场景改用 `QTemporaryFile` + `LoadSceneFromFile`
- **动画资产化（Unity 风格）**：
  - AnimatorDock 只允许 `.anim` 文件——移除内嵌剪辑编辑（纹理/网格/帧序列点选/时长/循环），每个状态**必须**引用 `.anim` 资产
  - Animator 运行时只使用 `.anim` 文件——`stateToJson` 移除 `clip` 序列化（只写 `assetPath`），`parseData` 从 `.anim` 文件加载剪辑到运行时缓存；旧场景含内嵌 clip 的状态向后兼容
- **CI 跨平台构建**：GitHub Actions 三平台工作流——Windows（MSVC + VS2022，BUILD_TOOLS=OFF）、Linux（GCC + Ninja，BUILD_TOOLS=ON，含编辑器）、macOS（Clang + Ninja，BUILD_TOOLS=ON，含编辑器），每平台 `cmake --install` 打包 SDK 上传 artifact
- **动画剪辑 + Animator 状态机（P28，引擎 + 编辑器）**：精灵动画从"运行时动态播放"升级为"可序列化、参数驱动的状态机"：
  - **引擎**：新增独立目录 `Animation/` —— `AnimationClip`（剪辑名/纹理路径/网格参数/每帧时长/循环/默认/帧序列，JSON 可序列化，可作 `.anim` 资产）与 `Animator`（**状态机组件**：状态集 + 转换 + 参数 float/bool/trigger；`setFloat`/`setBool`/`setTrigger` 参数驱动；BehaviorSystem 驱动 `onUpdate`——推进当前状态剪辑 + 求值转换，满足条件即切状态并重启动画）。`AnimationComponent`（可序列化多剪辑版）保留，`AnimationClip` 从其中迁至 `Animation/AnimationClip.h`
  - **序列化**：`Animator` 以反射字符串载体 `m_animatorData`（JSON：states/params/transitions）随 `.scene` 落盘，`onAfterDeserialize`/`onFieldChanged` 解析重建；`syncData()` 反向同步
  - **状态机 API**：`addState/setState/removeState`（首状态自动为入口）、`addParam/setParam/removeParam`、`addTransition/setTransition/removeTransition`（from=-1 任意状态）、`currentStateIndex/evaluateTransitions`；删当前状态自动停播、删参数连带清理引用它的转换条件、trigger 求值后自动消耗
  - **编辑器**：检查器为 Animator 渲染**专用状态机编辑器**（`animatorwidget`）——参数表（类型/名/值增删）、状态列表（入口标记）+ 选中状态剪辑编辑（纹理/网格/时长/循环/**帧序列点选**）、转换编辑（from→to + 条件增删）；AnimationComponent 仍渲染剪辑编辑器；均写回载体并接撤销 + dirty
  - **Unity 风格 Animator 窗口**（`animatordock` + `animatorgraphview`，独立右侧 Dock「窗口→Animator」）：QGraphicsView 可视化状态机图——状态方块节点（入口状态黄色标记）、转换箭头曲线、**右键从一个状态拖到另一个状态创建转换**（空白处引出=任意状态）、**拖节点移动**（坐标随 `.scene` 保存）、滚轮缩放、Delete 删除选中状态/转换；左侧参数面板（float/bool/trigger 增删与值）、底部选中状态/转换属性（剪辑编辑 + 帧序列点选 / from-to + 条件）；`AnimatorState` 新增 `graphX/graphY` 图坐标字段随载体序列化
  - **`.anim` 资产**：资源面板过滤 `.anim`，双击 → 解析 `AnimationClip` JSON 应用到选中对象 Animator 的当前状态（`onAnimOpenRequested`）
  - **示例**：`Examples/scenes/AnimatorTest.scene`（idle/run/jump 三状态 + speed/grounded/jump 三参数 + 4 条转换）+ `Examples/resource/run.anim` + `PlayerAnimatorController` 行为脚本（A/D→speed、Space→jump trigger 驱动切换）
- **Tilemap 瓦片地图（P27，引擎 + 编辑器）**：2D 关卡搭建落地——新增 `Tilemap` 组件（继承 RendererComponent），把瓦片集纹理（sprite sheet）按 `m_gridWidth×m_gridHeight` 网格铺排成地图：
  - **引擎**：`Tilemap` 组件反射字段（Texture/TileWidth/TileHeight/GridWidth/GridHeight/TileWorldSize）；瓦片 id（-1 空格 / ≥0 索引）映射到纹理源矩形；`onRender` 按相机裁剪 + 视野粗剔除绘制每格；网格数据以反射字符串 `m_gridData`（逗号分隔含尺寸头）持久化，运行时 `m_tiles`（vector<int>）高效存储，`onAfterDeserialize`/`onFieldChanged` 双向同步
  - **API**：`setTexturePath`/`setTileSize`/`setGridSize`/`setTile`/`getTile`/`clear`/`getTileCount` 等；修改自动同步 `m_gridData` 保证 `.scene` 往返不丢
  - **编辑器**：组件自动出现在「Add Component」菜单与检查器；选中含 Tilemap 的对象时视口绘制**半透明青色网格线**（辅助对齐）；**瓦片刷图**——左键+Shift 放置画笔瓦片、右键擦除，拖拽连续刷，接入撤销栈（一次拖拽一步撤销，快照型）
  - **序列化**：`m_gridData` 按 std::string 原生序列化，场景加载/撤销恢复经 `onAfterDeserialize` 重建网格
  - **瓦片选择面板（P27 增强，编辑器）**：新增「瓦片」Dock（底部标签组第三页）——选中含 Tilemap 的对象时读取其瓦片集纹理，按 `tileWidth × tileHeight` 切成网格缩略图显示，点击选中该瓦片 id 作为画笔（再点同一格取消），顶部「橡皮」按钮切到擦除模式（-1）；画笔经 `Viewport::setPaintTileId` 注入视口，视口左键+Shift 即可刷**任意瓦片**（原固定 id=0），默认未选不刷（-2）防误刷；无 Tilemap 选中时面板显示占位提示，对象销毁/场景重建自动清空
- **物理关节（P26，引擎 + 编辑器）**：Box2D 约束玩法落地——新增 `Joint2D` 组件 + `JointType` 反射枚举（Distance 距离/Revolute 铰链/Weld 焊接/Prismatic 滑动），挂在本对象刚体（bodyA）与 `connectedBody` 引用字段指定的另一刚体（bodyB）之间：
  - **引擎**：`Joint2D` 组件带反射字段（类型/锚点/各关节参数），`ComponentRef<RigidBody2D>` 引用字段（检查器可拖拽、序列化存 UUID、目标销毁自动失效）；`PhysicsSystem2D` 认领组件创建/销毁 Box2D 关节（`b2Create*Joint`），目标刚体未就绪时每帧补建（自愈，同刚体语义）；世界锚点换算两刚体本地锚点；字段改动/类型切换经 `rebuildJoint` 销毁重建；场景/世界销毁时重置关节标志防悬垂
  - **参数**：Distance（Length/Spring/Hertz/Damping）、Revolute（Motor/角度限位）、Prismatic（AxisAngle/滑动限位/MotorForce）、Weld（刚性）；`onFieldChanged` 统一重建
  - **编辑器**：组件自动出现在「Add Component」菜单与检查器（Type 下拉、Connected Body 拖拽引用、Anchor 与参数输入）；视口物理调试新增**关节可视化**（青色虚线连接线 + 锚点圆点，复用「碰撞体」开关）
  - **序列化**：枚举按 int32、引用按 UUID、锚点按 Vector2 由 Prefab 序列化原生支持，`.scene` 可完整保存/加载关节
- **碰撞体编辑手柄（P25，编辑器）**：视口内直接编辑碰撞体形状——选中含碰撞体的对象时（「碰撞体」开关开启），其轮廓黄色高亮并叠加白色手柄：Box 四角方块拖拽改尺寸（屏幕位移逆旋转到对象局部系，双边伸缩；Ctrl 4px 吸附，最小 2px）、Circle 右端方块拖拽改半径（拖到鼠标处，Ctrl 吸附）；经 `setSize/setRadius` 写入并即时同步 Box2D 形状，拖拽接入撤销栈与 dirty 标记（复用 Gizmo 信号）。与「碰撞体」开关联动（轮廓隐藏时手柄一并失活）
- **Prefab 预置资产（P25，引擎 + 编辑器）**：Unity 式资产化——场景树右键「存为预置…」把选中对象**含子树**存为 `.prefab` 文件（新增 `SceneSerializer::toJson(GameObject*)`：子树 DFS 父先于子，与 .scene 同构）；资源面板白名单加入 `.prefab`（双击或拖入视口 → `SceneSerializer::fromJson` 追加实例化，根对象重名自动去重后缀、拖入落点映射为根对象世界坐标、自动选中）；引擎侧零格式改动（复用场景加载器），入撤销栈 + dirty
- **场景树多选 + 批量删除（P25，编辑器）**：`ExtendedSelection`（Ctrl/Shift 点选）；新增 `selectedObjects()`；Del / 右键「删除对象 (N)」批量删除——快照收集后逐个 `removeGameObject`（不迭代中删除），`scene_camera` 混选时过滤并红字提示、其余照删；撤销与 dirty 一次记录整批
- **播放态调试（P25，编辑器）**：播放中检查器**只读**（Unity 语义）——`Inspector::setPlayMode` 统一递归禁用表单（字段控件 / Add Component / 移除按钮 / 对象名栏 / 引用控件），但每帧回读刷新照常（运行时值实时可见）；视口 Gizmo 与碰撞体手柄拖拽禁用（`Viewport::setEditEnabled`，拾取仍可用）；进入/退出播放自动切换

- **跨分辨率恢复窗口越界（P25e，编辑器）**：保存的窗口几何来自更大屏幕/更高 DPI 时，恢复后右侧「属性」/底部「资源 日志」Dock 被推到屏幕外不可见（此前误判为面板丢失，清注册表无效——Dock 一直都在，只是被推出屏）。修复：`restoreGeometry` 异步生效后（延迟 120ms）按可用屏幕**正溢出**判定（宽/高超出或左/上越界，容忍 -8 阴影边距）钳制窗口尺寸并居中；普通小窗口/多屏布局不受影响

- **复制/粘贴对象（P24，编辑器）**：「编辑 → 复制对象 / 粘贴对象」（Ctrl+C / Ctrl+V）：复制把选中对象经 `Prefab::Capture` 序列化为 JSON 存入编辑器内部剪贴板（不占系统剪贴板，避免与文本复制冲突），粘贴经 `Prefab::FromJson` + `instantiate` 实例化——组件（含 UUID/引用字段）完整保留、重名自动追加「 (1)（2）」去重、父级继承，粘贴后自动选中新对象；输入框/文本框聚焦时快捷键不劫持（场景搜索/重命名输入照常）
- **检查器完善（P24，编辑器）**：属性面板两处增强——**对象名编辑**：面板顶部新增对象名输入框（选中对象即回显，回车/失焦提交）；**移除组件**：组件标题栏右侧新增「✕」按钮（hover 红色提示），点击经引擎新增的非模板 API `GameObject::removeComponent(std::type_index)` 移除并重建面板（模板版 `removeComponent<T>()` 重构为转调它）。两项均接入撤销栈（撤销标签「移除组件」/「重命名」）与 dirty 标记；护栏：`TransformComponent` 是对象基础组件拒删、`scene_camera` 的相机组件是编辑器视口基础设施拒删（违规仅日志红字提示，不崩溃）
- **快捷创建菜单（P24，编辑器）**：场景树右键菜单新增「新建」子菜单，五个模板——空对象 / 精灵 / 相机 / UI Canvas / 文本：各模板自动挂载对应组件（精灵 = SpriteRenderer、相机 = CameraComponent、Canvas = UICanvas、文本 = UIText）；「文本」优先挂到场景已有 Canvas 下，无 Canvas 时顺带自动创建一个（Unity 式 UI 层级适配）；对象名自动去重，入撤销栈 + dirty 标记

- **属性检查器 Add Component（P23，编辑器）**：Unity 式"添加组件"入口——检查器组件列表底部新增虚线「Add Component」按钮，点击弹出组件类型菜单；场景树右键「添加组件」菜单重构为两处共用的工具头 `componentmenu.h`（`collectAddableComponentTypes` / `buildAddComponentMenu`），行为完全一致。新增特性：**已持有的组件类型置灰并标注「（已有）」**（替代原先重复添加被 `addComponentInstance` 静默丢弃的不明确行为）；添加走反射工厂 + `addComponentInstance`，检查器立即重建显示新组件，接入既有撤销栈（撤销标签「添加组件」/ Ctrl+Z 恢复）与 dirty 标记（编辑会话安全）

- **窗口布局重排（P21，编辑器）**：改 `createDocks()` 让「场景视口 / 运行视口」改为窗口中央的标签页叠放（`QTabWidget`，`scene_camera` / 游戏相机语义与之前一致），「资源 / 日志」在窗口底部标签页合并（`tabifyDockWidget`，拖标题栏可拆回独立 Dock）；左侧场景树、右侧属性检查器保留。布局持久化升级为**带版本号**（`QSettings::saveState(QMainWindow::SaveFullState, kLayoutVersion)` / `restoreState(…, kLayoutVersion)`），旧版布局不匹配时自动落回默认排列，重启不失真

- **「窗口」菜单（编辑器）**：菜单栏新增「窗口」，列出全部可停靠面板（场景 / 属性 / 资源 / 日志），勾选 = 显示——面板右上角关闭后可从菜单重新打开；复用 `QDockWidget::toggleViewAction()`，勾选状态与面板可见性自动同步，关面板/恢复布局后菜单状态不失真

- **组件 UUID + 组件引用字段（P20，引擎 + 扫描器 + 编辑器 + 示例）**：Unity 式"组件拖拽引用"基础设施——
  - **组件持久 UUID**：`Component` 基类新增 64 位随机 ID（`getUuid()/setUuid()`，`GenerateComponentUuid()`，0 保留为空引用），构造即分配；随 `.scene` 落盘（`Prefab::ComponentData.uuid`）——ID 跨编辑会话稳定；旧 `.scene` 无 `uuid` 字段时加载现场分配，向后兼容
  - **`ComponentRef<T>` 引用字段**（`GameObject/ComponentRef.h`）：字段内只存目标组件 UUID（8 字节），`get()/operator->/operator bool` 经当前场景索引懒解析；目标组件被移除/对象销毁后自动返回 nullptr——**永不悬垂**（与 `WeakComponentRef` 会话期弱引用互补：它是可序列化的持久引用）。运行时实例化（`Prefab::instantiate`）不恢复记录 ID，防复制实例共享 uuid 引用串线
  - **Scene UUID 索引**：`Scene::componentByUuid()` 公开查询 + 索引随组件生命周期维护（`registerComponent/unregisterComponent`、`GameObject` 挂载/卸下/清理全挂钩，覆盖不主动注册系统的组件如 TransformComponent）；随机 uuid 撞车自动重发并告警
  - **反射与序列化支持**：扫描器识别 `ComponentRef<具体组件类型>` 字段（剥 `Shit::` 前缀解析模板参数），生成 `.Ref("<目标类型>")` 链式标记；`FieldInfo::refType` 标识引用字段（`isReference()`）；Prefab 序列化按 8 字节 UUID 直读直写（0 = 空引用）
  - **检查器拖拽引用（编辑器）**：组件标题栏可拖拽（`ComponentHeaderLabel`，携带 UUID/类型名）；引用字段渲染为引用控件 `RefFieldWidget`（显示「对象名 (组件类型)」+「✕」清除按钮），接受组件头拖拽与**场景树对象拖入**（自动挑选第一个类型可赋值的组件；类型校验沿反射基类链做与运行期 dynamic_cast 同语义的判定）；编辑走既有 dirty 标记 + 撤销快照
  - 示例 `CoinDemo.h` 的计数文本改由 `ComponentRef<UIText>` 引用（检查器拖拽/序列化恢复，不再按名现查），`CoinCollect.scene` 各组件带 uuid 落盘验证全链路

### 修复

- **脚本工程构建配置随 SDK 引擎 DLL 形态（编辑器，工程收尾）**：`ScriptBuilder` 此前固定 `--config Debug`（MSVC）/ `-DCMAKE_BUILD_TYPE=Debug`（MinGW）——SDK 只含 Release 引擎（`ShitEngine.dll`，无 `-d`）时，Debug 插件与 Release 引擎跨 DLL 传 `std::string`（`_ITERATOR_DEBUG_LEVEL` 不一致）在 `RegisterPluginTypes` 阶段崩溃，即「MyGame 插件加载失败」根因。修复：`sdkBuildConfig()` 按 SDK bin/ 的引擎 DLL 形态探测（只有 `ShitEngine-d.dll` → Debug，否则 Release），MSVC 与 MinGW 分支统一套用；Debug SDK（仅 `-d`）行为不变
- **SDK 安装漏带第三方编解码 DLL（引擎）**：Engine install 清单只含 SDL3 四个 DLL，SDL_mixer/image 动态加载的编解码器（mpg123/vorbis/ogg/opus/FLAC/png/webp/tiff/xmp/wavpack/gme）不随 SDK 分发——独立 Runtime / 导出包运行缺 DLL 崩溃。修复：改为 `install(DIRECTORY bin/ … FILES_MATCHING *.dll)`（排除示例插件），SDK 自包含，且不再硬编码 MSVC/MinGW 前缀命名差异
- **编辑器反复闪退修复（P22，编辑器）**：定位到确定性崩溃——`EnginePreview::refreshCameras()` 的"上一帧游戏相机延续"逻辑先解引用后校验：`m_gameCam` 是跨帧缓存，播放中游戏逻辑销毁相机或切换场景（旧场景整体销毁）后即为悬垂指针，下一帧 tick 里 `prev->getOwner()->getName() != "scene_camera"`（`preview.cpp`）读取已释放内存（调试堆 0xDD 填充），`std::string::_Equal` 内访问 `0xffffffffffffffff` 触发访问违例——七份崩溃转储（14:24/14:25/21:38/21:39/21:40，`Editor.exe!EnginePreview::refreshCameras+0x13b` ← `tick+0x82`）栈完全一致。修复：延续前先用指针比对遍历当前场景确认相机仍存活（不触碰悬垂内存），通过后才解引用——场景切换/对象销毁后自动落回重新挑选，不再崩溃
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

- 全面代码审查确认的 **43 个 Bug**
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
