# 使用说明（1.0.1）

1.0.1 的使用说明与 1.0.0 完全一致，唯一变化是资产拖入关卡现已可用。完整说明见 [1.0.0 使用文档](../1.0.0/Usage.md)。

## 本版本修复点

- **从内容浏览器拖入关卡**：现在把 `UScenecapsAsset` 拖入关卡视口，会自动生成一个 `AScenecapsActor` 容器，设置集合资产并重建预览。
- 原因：编辑器模块 `PostEngineInit` 加载，晚于引擎的 Actor 工厂自动扫描，导致 `UScenecapsActorFactory` 未注册。现已在模块启动时显式注册到 `UPlacementSubsystem` 和 `GEditor->ActorFactories`，关闭时对称注销。

## 其余功能

场景集合与条目、变体与过渡、修改器、程序化生成、ISM/HISM、Data Layer、快照、Actor 池、`.scx` / `.scnb` 导入导出、HDRI 天空盒等用法均未变化，详见 [1.0.0 使用文档](../1.0.0/Usage.md)。
