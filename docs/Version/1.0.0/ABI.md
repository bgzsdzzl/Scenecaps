# ABI 与二进制兼容性（1.0.0）

## 基线

| 项目 | 约定 |
| --- | --- |
| 引擎 | Unreal Engine 5.8+ |
| 语言标准 | C++20（随 UE 5.8） |
| 导出宏 | Runtime `SCENECAPS_API`，Editor `SCENECAPSEDITOR_API` |
| 模块依赖 | Runtime 不依赖 Editor；Editor 依赖 Runtime |
| 版本标识 | `VersionName = 1.0.0`，`Version = 100`，`IsBetaVersion = false` |

## 兼容性策略

- **同一 MAJOR 版本内（1.x.y）**：保持公开 C++ / Blueprint API 与 ABI 向后兼容；新增功能以追加接口或可选参数为主。
- **跨 MAJOR 版本**：不保证 ABI 兼容，需要重新编译依赖 Scenecaps 的模块。
- **PATCH 版本**：只做缺陷修复，不改变公开类型布局与函数签名。
- **1.0.0** 是首个稳定 ABI 基线。

## 二进制使用注意事项

1. Runtime 与 Editor 模块必须来自同一次构建，不得混用不同版本的 DLL / `.lib`。
2. 升级 Unreal Engine 小版本或切换编译配置（Debug / Development / Shipping）后必须完整重编译插件。
3. 仅使用公开头文件（`Source/Scenecaps/Public`、`Source/ScenecapsEditor/Public`）；`Private` 目录中的类型不提供兼容承诺。
4. Blueprint 调用以 `UFUNCTION` / `UPROPERTY` 为准；反射名（函数名、属性名、类名）在同一 MAJOR 版本内保持稳定。
5. 资产序列化由 `UScenecapsAsset` 负责，`.scnb` / `.scx` 自定义格式在同一 MAJOR 版本内保持可读写。

## 已知边界

- Editor 模块中的视口与工具条接口仅存在于编辑器构建，Game / Server 目标不可链接。
- HDRI 天空盒配置保存在 `EditorPerProjectUserSettings`，属于用户级设置，不随资产分发。
- 程序化实例与 ISM 绑定的精确映射为运行时数据（`Transient`），不写入资产文件；重新生成后由生成流程重建。
