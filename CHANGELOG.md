# 更新日志

本文档记录所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

---

## [未发布]

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
- CVS.bDefaultLighting = false 彻底禁用默认光照
- 禁用 SkyLight、DirectionalLight、清理 SkyAtmosphere

### 修复
- 编辑器预览光照过亮（默认光照导致）
- 变体过渡 ApplyInterpolationToAsset/ISM 实现
- BiomeSampler GenerateToISM/HISM 实现
- DataLayer 代码用 WITH_EDITOR 包裹（Game 目标编译通过）
- SnapshotActorsAsSceneAsset Game 目标返回 nullptr

### 移除
- 编辑器预览中的全局 SkyLight、DirectionalLight、SkyAtmosphere

---

## [1.0.0] - 2024-XX-XX

### 新增
- 首个发布版本
- 场景集合资产系统
- 变体系统（稀疏覆盖）
- ISM/HISM 批量生成
- Blueprint 与 C++ API

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

1. `Scenecaps.uplugin` 更新 Version、VersionName
2. 更新 `CHANGELOG.md` 发布说明
3. 创建标签：`git tag v1.0.0`
4. 推送标签：`git push origin v1.0.0`
5. GitHub Actions 自动创建 Release 并上传产物
6. 更新 `CHANGELOG.md` 最终发布说明