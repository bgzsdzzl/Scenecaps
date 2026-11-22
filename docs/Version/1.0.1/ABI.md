# ABI 与二进制兼容性（1.0.1）

## 基线

| 项目 | 约定 |
| --- | --- |
| 引擎 | Unreal Engine 5.8+ |
| 语言标准 | C++20（随 UE 5.8） |
| 导出宏 | Runtime `SCENECAPS_API`，Editor `SCENECAPSEDITOR_API` |
| 模块依赖 | Runtime 不依赖 Editor；Editor 依赖 Runtime |
| 版本标识 | `VersionName = 1.0.1`，`Version = 101` |

## 兼容性结论

- **1.0.1 是 PATCH 版本**，仅修复编辑器模块的 Actor 工厂注册问题，未改动任何公开类型、函数签名或反射名称。
- API 与 ABI 与 1.0.0 **完全兼容**：用 1.0.0 编译的下游模块无需重新编译即可配 1.0.1 二进制（仍建议整包重编以避免引擎差异）。
- Game / Server 目标不受影响（修复仅涉及 Editor 模块）。

## 本次修复的二进制影响

- 仅 `ScenecapsEditor` 模块变动：`StartupModule` / `ShutdownModule` 注册与反注册 Actor 工厂，`ScenecapsActorFactory::PostSpawnActor` 增加幂等保护。
- Runtime 模块二进制与 1.0.0 一致。

## 通用注意事项

1. Runtime 与 Editor 模块必须来自同一次构建，不得混用不同版本的 DLL / `.lib`。
2. 升级 Unreal Engine 小版本或切换编译配置后必须完整重编译插件。
3. 仅使用公开头文件（`Source/Scenecaps/Public`、`Source/ScenecapsEditor/Public`）。
4. 资产序列化格式（`.uasset` / `.scnb` / `.scx`）与 1.0.0 一致。
