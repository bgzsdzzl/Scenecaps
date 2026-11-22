# API 参考（1.0.0）

插件由两个模块组成：

- `Scenecaps`（Runtime）：公开类型使用 `SCENECAPS_API` 导出，Game 目标可用
- `ScenecapsEditor`（Editor）：公开类型使用 `SCENECAPSEDITOR_API` 导出，仅编辑器

## Runtime 模块

### `UScenecapsAsset` — 场景集合资产

| 成员 | 说明 |
| --- | --- |
| `Entries` / `Variants` / `RootId` | 条目、变体与根节点 ID |
| `GetVariantNames()` | 获取全部变体名 |
| `ApplyVariantToEntry(Base, VariantName)` | 将变体稀疏覆盖应用到条目副本 |
| `InterpolateEntry(...)` / `InterpolateAllEntries(...)` | 变体之间插值 |
| `BeginTransition(...)` | 构造过渡描述 |
| `SpawnCollection(...)` | 生成条目 Actor |
| `SpawnCollectionEx(...)` | 生成并输出 `EntryId -> AActor*` 映射 |
| `SpawnCollectionAsISM(...)` / `SpawnCollectionAsHISM(...)` | 批量实例化，返回首个 ISM/HISM 组件 |
| `GenerateProceduralTransforms(...)` | 程序化变换（网格、泊松圆盘、噪声聚类、径向） |

### `AScenecapsActor` — 场景集合 Actor

| 成员 | 说明 |
| --- | --- |
| `CollectionAsset` / `ActiveVariant` / `bIncludeHidden` | 集合资产、激活变体与隐藏条目开关 |
| `Rebuild()` / `Clear()` | 重建 / 清空生成结果 |
| `SetCollectionAsset(...)` / `SetActiveVariant(...)` | 运行时切换（可选立即重建） |
| `GetSpawnedActorArray()` / `FindSpawnedActorsByClass(...)` | 查询生成的 Actor |
| `GetSpawnedEntries()` | 条目 ID → 生成 Actor 的绑定（供过渡使用） |
| `SetISMContainerActor(...)` / `GetISMContainerActor()` | 批量实例化容器关联 |
| `ApplyModifiers(...)` / `ApplyModifiersToActor(...)` | 执行修改器 |

### `UScenecapsISMComponent` — 批量实例管理组件

| 成员 | 说明 |
| --- | --- |
| `RegisterContainerActor(Actor)` | 注册容器并收集其中的 ISM/HISM |
| `RegisterEntryInstance(...)` | 注册条目 → 实例绑定（由生成流程自动调用） |
| `GetEntryBindings()` / `GetEntryBindingCount()` | 查询实例绑定 |
| `AddInstanceToISM/HISM(...)`、`UpdateInstanceTransformISM/HISM(...)` | 实例增删与变换 |
| `SetInstanceCustomDataArrayISM/HISM(...)` | 逐实例 CustomPrimitiveData |

### `UScenecapsVariantTransitionComponent` — 变体过渡组件

| 成员 | 说明 |
| --- | --- |
| `TransitionAsset` / `FromVariant` / `ToVariant` | 目标资产与变体 |
| `Duration` / `EaseType` / `bLoop` / `bPingPong` / `bAutoPlay` | 过渡参数 |
| `Play()` / `PlayFromTo(...)` / `Stop()` / `Pause()` / `Resume()` / `Reverse()` | 播放控制 |
| `SetTime(...)` / `SetAlpha(...)` | 定位到指定时间 / 插值系数 |
| `ApplyCurrentInterpolation()` | 手动应用一次插值 |
| 事件 | `OnTransitionStarted` / `OnTransitionProgress` / `OnTransitionEnded` / `OnTransitionLooped` / `OnTransitionPingPong` |

### 其他 Runtime 类型

- `UScenecapsModifierManager` 与内置修改器：`UScenecapsTransformModifier`、`UScenecapsMaterialModifier`、`UScenecapsVisibilityModifier`、`UScenecapsCustomParamsModifier`、`UScenecapsActorPropertyModifier`
- `IScenecapsEntryHandler` / `FScenecapsEntryRegistry`：条目资产处理器注册与分派
- `UScenecapsActorPoolComponent`：`AcquireActor()`、`ReleaseActor()`、`PrewarmPool()`
- `UScenecapsBiomeSamplerComponent` / `UScenecapsBiomeAsset`：生物群系采样与生成
- `UScenecapsSnapshotRecorder`：场景快照与回填
- `FScenecapsSpawnContext`：条目生成上下文（变换、材质、参数、CustomPrimitiveData、EntryId）

## Editor 模块

- `FScenecapsEditorToolkit`：资产编辑器入口，提供 `ApplyHDRISkyboxToPreview()`、`SetHDRISkyboxEnabled()`、`SetHDRISkyboxTexture()` 等视口环境接口
- `SScenecapsEditorViewport` / `FScenecapsEditorViewportClient`：视口控件与客户端
- `UScenecapsViewportSettings`：视口配置（`config = EditorPerProjectUserSettings`），含 HDRI 天空盒开关与贴图
