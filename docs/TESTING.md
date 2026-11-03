# 测试要求

本文档定义 Scenecaps 项目的测试标准与流程。

---

## 测试层级

| 层级 | 范围 | 工具/方法 |
|------|------|-----------|
| 单元测试 | 独立函数/类 | 自动化测试框架（待引入） |
| 集成测试 | 模块交互 | Editor 自动化测试 / Gauntlet |
| 端到端测试 | 完整工作流 | 手动 + 自动化回归 |
| 回归测试 | 核心功能 | 每次 PR 运行 |

---

## 必测配置矩阵

| 目标 | Editor Development | Game Development | Game Shipping |
|------|-------------------|------------------|---------------|
| Runtime 模块 | ✅ | ✅ | ✅ |
| Editor 模块 | ✅ | ❌ | ❌ |
| Niagara 启用 | ✅ | ✅ | ✅ |
| Niagara 禁用 | ✅ | ✅ | ✅ |
| World Partition | ✅ | ✅ | ✅ |

**所有 PR 必须在上述所有配置通过编译。**

---

## 功能测试清单

### 场景集合核心

| 测试项 | 预期结果 |
|--------|----------|
| 创建 ScenecapsAsset | 资产创建成功，RootId 生成 |
| 添加/删除/移动条目 | 层级结构正确，ParentId 更新 |
| 变体创建/切换 | 仅 Overrides 生效，Base 不变 |
| 变体插值 | Transform/Material/Params 平滑过渡 |
| Rebuild() | 场景按当前变体重新生成 |
| Clear() | 所有 SpawnedActors 销毁 |

### 变体系统

| 测试项 | 预期结果 |
|--------|----------|
| 创建/删除/重命名变体 | 变体列表更新 |
| 添加条目到变体 | Override 生成，仅受影响字段 |
| 从变体移除条目 | Override 移除，回退 Base |
| 运行时切换变体 | Actor 实时更新 Transform/Visibility/Material |
| 变体过渡组件 | Alpha 0→1 平滑插值，支持 Loop/PingPong |

### ISM/HISM 批量生成

| 测试项 | 预期结果 |
|--------|----------|
| SpawnCollectionAsISM | 相同 Mesh 合并为单 ISM，实例数正确 |
| SpawnCollectionAsHISM | HISM 创建，LOD/树构建正常 |
| 运行时 ISM 操作 | Add/Remove/UpdateInstance 实时生效 |
| Lumen 兼容 | Distance Field / Indirect Lighting 正常 |

### 程序化生成

| 测试项 | 预期结果 |
|--------|----------|
| BiomeAsset 生成 | 按坡度/高度/遮罩筛选 |
| Grid/Poisson/Noise/Radial | 分布符合参数 |
| Seed 确定性 | 相同 Seed 生成相同结果 |
| GenerateToISM/HISM | 直接生成 ISM/HISM，无中间 Actor |

### Data Layer 集成

| 测试项 | 预期结果 |
|--------|----------|
| Entry 指定 DataLayer | 生成 Actor 自动加入对应 DataLayer |
| Actor 级 DataLayer | 优先级高于 Entry 级 |
| bSpawnOnDataLayerLoaded | DataLayer 激活时自动 Rebuild |
| 运行时切换 DataLayer | Actor 隐藏/显示同步 |

### 快照记录器

| 测试项 | 预期结果 |
|--------|----------|
| SnapshotWorldAsSceneAsset | 捕获关卡 Actor 层级/变换/材质 |
| 保存为 Asset | 可在 Content Browser 打开 |
| 恢复快照 | SpawnSceneSnapshot 恢复原貌 |

### Actor 池

| 测试项 | 预期结果 |
|--------|----------|
| Acquire/Release | 复用 Actor，避免频繁 Spawn/Destroy |
| Prewarm | 预热指定数量 |
| CleanupIdle | 超时自动销毁闲置 Actor |

### 修改器系统

| 测试项 | 预期结果 |
|--------|----------|
| Register/Unregister | 优先级排序生效 |
| Phase 执行顺序 | PreSpawn → PostSpawn → PerEntry |
| TransformModifier | Location/Rotation/Scale 叠加 |
| MaterialModifier | MaterialOverride 应用到指定 Slot |
| VisibilityModifier | Tag 筛选 + bInvert 反转 |

### 编辑器工具

| 测试项 | 预期结果 |
|--------|----------|
| Outliner 拖拽重排 | ParentId 更新，层级正确 |
| 变体下拉切换 | 预览实时更新 |
| 批量编辑 | 多选属性同步修改 |
| 拖拽资产创建 Entry | 自动识别 Handler，生成 Entry |
| 快照录制 | 选中 Actor → 生成 Scene Asset |
| 变体时间线 | 滑块拖动实时预览插值 |

---

## 自动化测试

### 编译验证（每次 PR）

```bash
# Editor Development

RunUAT.bat BuildPlugin -Plugin=... -TargetPlatforms=Win64 -TargetConfigurations=Development -Build=UnrealEditor


# Game Development
... -Build=UnrealGame -TargetConfigurations=Development


# Game Shipping
... -Build=UnrealGame -TargetConfigurations=Shipping

```


### 静态分析

- MSBuild 警告视为错误 (`TreatWarningsAsErrors=true`)
- Clang-Tidy 检查（可选集成）

### 内存检查

```bash
# 编辑器中

obj list class=Scenecaps

obj refs class=ScenecapsActor
```


- 无内存泄漏（退出时对象计数归零）
- 无 GC 引用循环

---

## 性能基准

| 场景 | 基准 | 通过标准 |
|------|------|----------|
| 10k 实例 ISM 生成 | < 2s | < 3s |
| 变体切换 (1k Actor) | < 100ms | < 200ms |
| 变体过渡 (1s) | 60 FPS | ≥ 55 FPS |
| 快照记录 (1k Actor) | < 1s | < 2s |
| Actor 池 Acquire/Release | < 1ms | < 2ms |

---

## 回归测试清单

每次发布前必须验证：

- [ ] 空场景创建/加载/保存
- [ ] 100+ 条目场景 Rebuild/Clear
- [ ] 5 个变体互相切换
- [ ] 变体过渡播放/暂停/循环
- [ ] ISM/HISM 生成/销毁
- [ ] 生物群系生成 (Seed 固定)
- [ ] DataLayer 分配/切换
- [ ] 快照录制/恢复
- [ ] Actor 池 预热/获取/释放
- [ ] 修改器注册/执行/卸载
- [ ] Niagara 启用/禁用均正常
- [ ] 编辑器 Outliner/视口/详情面板交互
- [ ] 打包 Shipping 配置启动无崩溃

---

## 已知问题与规避

| 问题 | 状态 | 规避方案 |
|------|------|----------|
| 大量 Actor 选中时编辑器卡顿 | 已知 | 分批选择 / 使用 Outliner 搜索 |
| 变体过渡中材质插值闪烁 | 调查中 | 临时禁用材质插值 |
| Niagara 粒子在 ISM 上不渲染 | 引擎限制 | 使用单独 Actor 承载 Niagara |

---

## 测试环境

- UE 5.8.0+
- Windows 10/11 (x64)
- Visual Studio 2022 17.8+ / Rider 2023+
- GPU: 支持 DX12 / Vulkan
- 内存 ≥ 16GB（建议 32GB+）
