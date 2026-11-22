# 更新日志

本文档记录所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

---

## [1.0.0] - 2026-11-22

首个正式发布版本。版本文档（API、ABI、教程、使用说明）见 [docs/Version/1.0.0](docs/Version/1.0.0/README.md)。

### 新增

- 编辑器资产视口 HDRI 天空盒：工具条手动选择 TextureCube 后启用，默认关闭，配置持久化
- 程序化生成正式接入 `SpawnCollectionEx`，条目可一次生成多个实例（网格、泊松圆盘、噪声聚类、径向）
- `UScenecapsActorPropertyModifier` 属性覆盖实现（FProperty 反射，支持组件名与组件标签）
- `FScenecapsExtensionManager` 扩展 DLL 扫描、加载与释放
- World Partition：DistanceBased 规则参考点回退链，HLOD 级别真实生效
- Biome 按加权选中的 Mesh / 材质分组生成 ISM / HISM
- 快照记录器：多 Mesh / 材质槽选取，变体生成真实 overrides
- ISM / HISM 转换保留 CustomPrimitiveData

### 变更

- 变体过渡 ISM 路径重新设计：新增 `FScenecapsISMEntryBinding` 条目→实例精确绑定，场景 Actor 关联批量容器，兼容嵌套层级与程序化多实例
- 新增 `docs/Version/<版本号>/` 版本文档体系，按版本归档 API、ABI、教程与使用说明

### 修复

- 运行时 Game 目标不生成集合（`BeginPlay` 被 `WITH_EDITOR` 包裹）
- `FTransform` 组合顺序错误，导致运行时变换与编辑器预览不一致
- 变体过渡所有 Actor 被最后一个条目覆盖、反向播放双重反转
- ISM / HISM 过渡隐藏实例时位置被重置到原点、多条目抢用同一实例
- CustomPrimitiveData 槽位冲突、四元数插值未归一化
- 数据驱动 Handler 缓存错误组件类的 `FProperty`（内存写错风险）
- `.scnb` 导入无长度校验（OOM）、导出丢失非类资产引用
- Actor 池使用 `IsValidLowLevel`、条目专属 DataLayer 残留
- 生物群系坡度射线忽略目标 Landscape
- AssetDefinition 未反注册（热重载残留）

---

## [0.9.0] - 2026-11-05

### 新增

- 编辑器预览视口：每物体点光源（无全局天空/大气）
- 变体过渡系统：支持插值
- 生物群系程序化生成（网格、泊松圆盘、噪声聚类、径向）
- Data Layer 集成（World Partition）
- 快照记录器（捕获关卡 Actor 为场景集合）
- Actor 池（运行时对象池）
- 数据驱动条目处理器（.scx 格式）
- 修改器系统（基于阶段执行）
- 变体过渡组件（支持插值）

### 变更

- 编辑器预览：完全移除天空/大气/全局光
- 编辑器预览：每物体点光源（强度 150、半径 300）
- `CVS.bDefaultLighting = false` 彻底禁用默认光照
- 禁用 SkyLight、DirectionalLight，清理 SkyAtmosphere

### 修复

- 编辑器预览光照过亮（默认光照导致）
- 变体过渡 `ApplyInterpolationToAsset` / `ApplyInterpolationToISM` 实现
- BiomeSampler `GenerateToISM` / `GenerateToHISM` 实现
- DataLayer 代码用 `WITH_EDITOR` 包裹（Game 目标编译通过）
- `SnapshotActorsAsSceneAsset` Game 目标返回 `nullptr`

### 移除

- 编辑器预览中的全局 SkyLight、DirectionalLight、SkyAtmosphere

---

## 版本规范

遵循 [语义化版本](https://semver.org/lang/zh-CN/)：

- **MAJOR**：不兼容 API 变更
- **MINOR**：向后兼容的新功能
- **PATCH**：向后兼容的缺陷修复

### 发布类型

- **Major** (x.0.0)：不兼容 API 变更、架构重大调整
- **Minor** (x.y.0)：向后兼容的新功能
- **Patch** (x.y.z)：向后兼容的缺陷修复

### 预发布版本

- **Alpha** (x.y.z-alpha.N)：早期测试，不稳定
- **Beta** (x.y.z-beta.N)：功能完整，测试中
- **RC** (x.y.z-rc.N)：准备发布，最终测试

---

## 发布流程

1. `Scenecaps.uplugin` 更新 Version、VersionName，并关闭 `IsBetaVersion`
2. 更新 `CHANGELOG.md` 发布说明与 `docs/Version/<版本号>/` 版本文档
3. 确认工作区干净并已提交
4. 创建标签：`git tag v1.0.0`
5. 推送标签：`git push origin v1.0.0`
6. GitHub Actions 自动创建 Release 并上传产物
