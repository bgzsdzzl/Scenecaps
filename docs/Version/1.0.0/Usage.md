# 使用说明（1.0.0）

## 场景集合与条目

- 条目描述一个对象及其相对变换，由 `FGuid EntryId` 标识；`RootId` 代表集合根。
- 生成时按 `ParentId` 建立父子挂接，`bIncludeHidden` 控制是否生成隐藏条目。
- 集合的重建是非破坏性的：原始资产与关卡对象不会被修改，仅依据集合定义重新生成。

## 变体与过渡

- 变体是稀疏覆盖：只有 `bOverrideXxx` 为真的字段生效。
- `AScenecapsActor::SetActiveVariant()` 可运行时切换，传入 `bRebuildImmediately` 决定是否立即重建。
- 过渡组件支持 `Loop` / `PingPong` / `Reverse`，事件包括 Started / Progress / Ended / Looped / PingPong。
- 反转播放不会把 Alpha 与变体顺序重复反转；ISM 路径按 `EntryId → 实例` 绑定精确应用。

## 修改器

- 修改器按 `Phase` 执行，可挂到 `AScenecapsActor` 的 `ModifierManager`。
- 内置变换、材质、可见性、自定义参数与 Actor 属性修改器。
- Actor 属性修改器支持 `组件名.属性名`，并可配合 `ComponentTags` 批量应用。

## 程序化生成

- `ProceduralParams` 有效时，一个条目会按网格、泊松圆盘、噪声聚类或径向分布生成多个实例。
- 可通过 `Density`、`Seed`、`MinDistance`、`GridSpacing`、`ClusterRadius` 等参数控制。
- 随机缩放、旋转偏移与位置抖动在生成阶段统一应用。

## ISM / HISM 批量生成

- `SpawnCollectionAsISM` / `SpawnCollectionAsHISM` 会创建独立容器 Actor，并挂 `UScenecapsISMComponent`。
- 条目生成时写入的 CustomPrimitiveData 会保留到实例上；变体过渡会从基线恢复后再写入插值参数。
- 场景 Actor 通过 `SetISMContainerActor()` 记住容器，过渡组件据此定位管理组件。
- `NumLODs` / `LODScreenSize` 在 UE 5.8 没有组件级 API，LOD 由 StaticMesh 的 LOD 组决定。

## Data Layer（编辑器）

- `TargetDataLayer` 与条目自身的 `DataLayer` 均可指定；条目优先。
- 清理时只移除本次实际添加过的层，避免残留。
- Game 目标没有等价的运行时 Data Layer 处理，需要显式触发重建。

## 快照记录器

- `SnapshotActorsAsSceneAsset()` 把 Actor 列表捕获为集合资产（仅编辑器）。
- 会选取第一个有效 MeshComponent 的资产与材质，并记录 Mobility / Collision / CastShadow 覆盖。
- `bAutoGenerateVariant` 会为每个条目生成记录当前状态的 overrides。

## Actor 池

- `AcquireActor(PoolKey, Transform, Owner)` / `ReleaseActor(Actor, PoolKey)` 成对使用。
- `PrewarmPool()` 预热；`MaxPoolSizePerClass` 与 `MaxIdleTimeSeconds` 控制容量与回收。

## 数据驱动条目（.scx）

- 支持用文本描述 Actor 类、组件与属性覆盖。
- 组件属性按组件名解析并缓存 `FProperty`，运行时按名字定位真实组件后写入。

## 导入 / 导出（.scnb）

- 导出写入条目的完整软对象路径，保留 Mesh / 材质 / 蓝图等资产引用。
- 导入对字符串长度与条目数量做上限校验，损坏文件不会造成巨量分配。

## 视口 HDRI 天空盒

- 默认关闭；必须在资产编辑器视口工具条中手动选择 `TextureCube` 后才会启用。
- 配置位于 `EditorPerProjectUserSettings`，可通过 `UScenecapsViewportSettings` 调整强度。
- 启用时设置预览场景 SkyLight 为 `SLS_SpecifiedCubemap`，并开启视口 SkyLighting 标志。

## 注意事项

- Runtime 模块不依赖 Editor 模块，Game 目标可单独使用。
- 公开类型通过 `SCENECAPS_API` 导出，ABI 约定见 [ABI.md](ABI.md)。
- 升级引擎版本或切换构建配置后需要完整重编译插件。
