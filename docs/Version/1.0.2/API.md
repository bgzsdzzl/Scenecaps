# API 参考（1.0.2）

1.0.2 是缺陷修复版本，**公开 API 与 1.0.1 / 1.0.0 完全一致**，无任何新增、删除或签名变更。完整 API 清单见 [1.0.0 的 API 文档](../1.0.0/API.md)。

## 模块与导出

- `Scenecaps`（Runtime）：公开类型使用 `SCENECAPS_API` 导出，Game 目标可用
- `ScenecapsEditor`（Editor）：公开类型使用 `SCENECAPSEDITOR_API` 导出，仅编辑器

## 本版本涉及的类型（行为修复，签名不变）

| 类型 | 变更 |
| --- | --- |
| `FScenecapsEditorToolkit` | 新增私有 `EnsureHDRISkySphere()` 与天空球成员；`ApplyHDRISkyboxToPreview` 改为天空球方案。均为私有实现，不涉及公开 API |
| `FScenecapsEditorViewportClient` | 构造函数改用 `DisableAdvancedFeatures()`，`Tick` 中刷新捕获；移除 `SetSkyLighting` 运行时切换 |

## 核心 API 快速索引

- 资产：`UScenecapsAsset`（`SpawnCollection` / `SpawnCollectionEx` / `SpawnCollectionAsISM` / `SpawnCollectionAsHISM`）
- 场景 Actor：`AScenecapsActor`（`Rebuild` / `Clear` / `SetActiveVariant` / `GetSpawnedEntries`）
- 批量实例：`UScenecapsISMComponent`
- 过渡：`UScenecapsVariantTransitionComponent`
- 修改器：`UScenecapsModifierManager` 及内置修改器
- 其他：`UScenecapsActorPoolComponent`、`UScenecapsSnapshotRecorder`、`UScenecapsBiomeSamplerComponent`

详见 [1.0.0 API 文档](../1.0.0/API.md)。
