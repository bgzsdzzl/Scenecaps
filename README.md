# Scenecaps / 场景胶囊

面向 Unreal Engine 5.8+ 的非破坏性场景集合与关卡组装插件。

Scenecaps 以「场景集合资产」为核心，将关卡中的多个对象组织为可复用、可切换、可程序化生成的条目集合，并提供变体、批量实例化与运行时过渡能力。

---

## 一、简介

在传统的关卡制作中，同一组对象的重复摆放、变体切换与批量管理往往依赖手工操作，容易产生冗余、难以维护。Scenecaps 将这类重复工作抽象为数据驱动的流程：

- 将对象描述为「条目」，组织进「场景集合资产」；
- 通过「变体」对集合进行稀疏覆盖，而不复制整体；
- 通过「修改器」在生成阶段对条目进行统一处理；
- 通过 ISM/HISM 批量实例化降低运行时开销。

集合的构建与调整是非破坏性的：原始资产与关卡对象不会被修改，重建仅依据集合定义重新生成结果。

---

## 二、功能特性

| 功能 | 说明 |
| --- | --- |
| 场景集合 / Scene Collection | 从模块化条目非破坏性组装场景 |
| 变体系统 / Variant System | 稀疏覆盖与运行时插值 |
| ISM/HISM 批量实例化 | 自动实例化以提升性能 |
| 变体过渡 / Variant Transitions | 变体之间的平滑插值过渡 |
| 生物群系程序化生成 | 网格、泊松圆盘、噪声聚类、径向分布 |
| Data Layer 集成 | 在编辑器中与 World Partition 协作 |
| 修改器系统 / Modifier System | 基于阶段的可扩展修改器 |
| 快照记录器 / Snapshot Recorder | 捕获关卡对象为场景集合 |
| Actor 池 / Actor Pool | 运行时对象复用 |

> [!NOTE]
> Data Layer 的加载状态触发重建目前仅在编辑器中生效。Game 目标尚未提供等价的运行时 Data Layer 处理，运行时需通过显式调用触发重建。

---

## 三、环境要求

- Unreal Engine 5.8 或更高版本；
- 编辑器平台：Windows、macOS 或 Linux；
- 目标平台：Win64、Linux、Mac；
- 编译器：Windows 使用 Visual Studio 2022 17.8+，其他平台使用兼容的 Clang 工具链；
- Niagara（可选，用于粒子类条目，可通过构建环境变量关闭）。

插件同时提供 Runtime 模块与 Editor 模块，可在 Game 目标中仅保留 Runtime 功能。

---

## 四、安装

1. 将插件目录放入工程的 `Plugins/` 下；
2. 重新生成工程文件；
3. 编译并在编辑器的插件面板中启用 Scenecaps。

若以源码形式集成，请确保 `Scenecaps.uplugin` 中声明的模块与 `Source/` 下的目录结构一致。

---

## 五、快速上手

1. 创建「场景集合资产」，作为条目与变体的容器；
2. 向集合中添加条目，每个条目描述一个对象及其变换；
3. 需要时创建变体，仅覆盖发生变化的字段；
4. 将 `AScenecapsActor` 放入关卡，指定集合资产；
5. 调用重建，或由变体切换触发重建；在编辑器中，Data Layer 的加载状态变化也会触发重建。

程序化生成、快照记录与 Actor 池为可选能力，按需接入。

---

## 六、项目结构

```text
Source/
├── Scenecaps/          # Runtime 模块
│   ├── Public/         # 公开 API
│   └── Private/        # 内部实现
└── ScenecapsEditor/    # Editor 模块
    ├── Public/
    └── Private/
```

公开类型通过 `SCENECAPS_API` 导出。Runtime 模块不依赖 Editor 模块。

---

## 七、文档索引

| 文档 | 内容 |
| --- | --- |
| [贡献指南](CONTRIBUTING.md) | 参与开发、文档与反馈的流程 |
| [编码规范](docs/CODING_STANDARDS.md) | C++ 编码与风格约定 |
| [测试要求](docs/TESTING.md) | 测试层级与验证标准 |
| [Pull Request 流程](docs/PR_PROCESS.md) | 分支、提交、审核与合并 |
| [生成式 AI 使用方针](docs/AI_USAGE_POLICY.md) | AI 使用披露与审查 |
| [AI Agent 使用指南](docs/AI_AGENTS_GUIDE.md) | Agent 行为边界与日志 |
| [行为准则](CODE_OF_CONDUCT.md) | 协作行为标准 |
| [更新日志](CHANGELOG.md) | 版本变更记录 |

---

## 八、贡献

欢迎通过 Issue 报告缺陷或提出建议，也欢迎提交 Pull Request。

在提交贡献前，请阅读 [贡献指南](CONTRIBUTING.md) 与相关规范文档。提交代码即表示同意按项目许可发布。

---

## 九、许可

本项目采用 Apache License 2.0 许可，详见 [LICENSE](LICENSE)。
