# Pull Request 流程

本文档规定 Scenecaps 项目的分支策略、提交规范、Pull Request 提交与审查、合并与发布，以及仓库保护规则。

本文档描述的是正常协作路径。紧急修复、安全报告等特殊情况，见对应章节。

---

## 一、总则

- 所有进入受保护分支的改动，都应通过 Pull Request 进行；
- Pull Request 应聚焦单一目标，便于独立审查与验证；
- 提交信息、签名与披露应满足仓库规则集的要求；
- 改动在合并前必须通过持续集成检查。

---

## 二、分支策略

| 分支 | 用途 | 保护 |
| --- | --- | --- |
| `main` | 稳定发布分支 | 受保护，仅允许 PR 合并 |
| `develop` | 开发集成分支 | 受保护，仅允许 PR 合并 |
| `feature/*` | 新功能开发 | 临时，合并后删除 |
| `fix/*` | 缺陷修复 | 临时，合并后删除 |
| `docs/*` | 文档更新 | 临时，合并后删除 |
| `refactor/*` | 重构 | 临时，合并后删除 |
| `test/*` | 测试相关 | 临时，合并后删除 |
| `release/*` | 发布准备 | 临时，合并后删除 |
| `hotfix/*` | 紧急修复 | 从 `main` 切出，合并回 `main` 与 `develop` |

分支名应简洁描述改动内容，例如 `feature/biome-radial-distribution`、`fix/variant-transition-crash`。

---

## 三、提交规范

### 1. 提交信息格式

提交信息遵循 Conventional Commits：

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

常用类型：

| 类型 | 含义 |
| --- | --- |
| `feat` | 新功能 |
| `fix` | 缺陷修复 |
| `docs` | 文档更新 |
| `refactor` | 重构（无功能变更） |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `chore` | 构建、工具或依赖调整 |
| `revert` | 回滚 |
| `ci` | 持续集成配置 |

示例：

```text
feat(editor): add per-object point lights to preview viewport

Disable default lighting and add per-object point lights
to avoid overbright preview.

Closes #123
```

### 2. 提交签名与签核

进入受保护分支的提交必须满足：

- **GPG 签名**：提交须带有有效签名，GitHub 会将其标记为 Verified；
- **签核行**：提交信息末尾须包含 `Signed-off-by` 行，格式为：

```text
Signed-off-by: Name <email@example.com>
```

推荐使用 `git commit -s` 自动追加签核行。仓库规则集会对未签名或缺少签核行的提交进行拦截。

### 3. 提交范围

保持每次提交聚焦。不要在一次提交中混入无关的格式化、重命名或迁移。大规模改动应拆分为可独立审查的多个提交或 PR。

---

## 四、Pull Request 流程

### 1. 创建前

- [ ] 基于最新目标分支创建改动分支；
- [ ] 同步上游最新代码；
- [ ] 本地编译通过（Development 与 Shipping）；
- [ ] 完成与改动相适应的功能验证；
- [ ] 代码符合 [编码规范](CODING_STANDARDS.md)；
- [ ] 已按 [生成式 AI 使用方针](AI_USAGE_POLICY.md)披露 AI 使用情况。

### 2. 创建

- **Base 分支**：功能与修复指向 `develop`，紧急修复指向 `main`；
- **标题**：遵循 Conventional Commits；使用 AI 辅助完成的 PR，需在标题末尾添加模型标识；
- **描述**：按 Pull Request 模板填写，说明变更类型、动机、范围与验证方式。

### 3. 描述应包含的内容

| 项目 | 说明 |
| --- | --- |
| 变更类型 | 对应提交类型 |
| 变更描述 | 解决什么问题、主要实现方式 |
| 相关 Issue | 关联的 Issue 编号 |
| 测试验证 | 已完成的编译与功能验证 |
| 影响范围 | 涉及的模块、公开 API、数据格式 |
| AI 使用情况 | 使用情况、工具与模型、人工审查 |

---

## 五、审查

### 1. 审查者职责

审查者应关注：

- 改动的逻辑正确性；
- 是否符合编码规范与项目架构；
- 是否存在性能、内存或兼容性问题；
- 测试与验证是否充分；
- 文档是否同步更新。

### 2. 审查结论

审查者可以给出以下结论：

- **Approve**：同意合并；
- **Request Changes**：需修改后重新审查；
- **Comment**：提出意见，不阻塞。

被 Request Changes 时，贡献者应推送修改到同一分支，PR 会自动更新并由审查者重新审查。所有讨论应在合并前解决。

---

## 六、合并

| 场景 | 合并方式 |
| --- | --- |
| 常规功能或修复 | Squash and merge |
| 需要保留历史的关联提交 | Rebase and merge |
| 紧急修复 | Squash 到 `main`，再 cherry-pick 到 `develop` |

合并前检查：

- [ ] 全部必需的状态检查通过；
- [ ] 无未解决的 Request Changes；
- [ ] 满足所需的 Approve 数量；
- [ ] 分支已与目标分支同步，无冲突；
- [ ] 所有讨论线程已解决。

合并后，源分支应删除，关联 Issue 通过关键字自动关闭。

---

## 七、发布与热修复

### 1. 版本发布

1. **准备**：创建 `release/vX.Y.Z` 分支，更新插件版本号与更新日志；
2. **验证**：全配置构建通过，核心功能冒烟测试，打包配置验证；
3. **发布**：推送标签，由持续集成创建 Release 并上传产物；
4. **收尾**：更新更新日志中的最终发布说明。

### 2. 热修复

1. 从 `main` 切出 `hotfix/vX.Y.Z`；
2. 修复并验证；
3. PR 合并回 `main`；
4. 推送新标签；
5. cherry-pick 到 `develop`。

---

## 八、回滚

发现严重回归时，可采用以下方式：

```bash
# 回滚某次合并
git revert <merge-commit-sha> -m 1
git push origin main
```

回滚同样通过 Pull Request 进行，不应直接向受保护分支推送。

---

## 九、仓库保护规则

受保护分支通过仓库规则集进行约束，通常包括：

- 要求提交签名与签核；
- 要求通过 Pull Request 合并；
- 要求指定数量的审核通过；
- 要求必需的状态检查通过；
- 要求分支在合并前保持最新；
- 要求所有讨论解决。

持续集成通常包含以下检查：

| 检查 | 内容 |
| --- | --- |
| Build Validation | 模块结构、依赖声明与导出宏 |
| Include Check | 头文件引用可解析 |
| Code Quality | Markdown 格式与文档链接 |
| Security | 密钥扫描与依赖审查 |
| Project Structure | 基于 `.uplugin` 的结构一致性 |

---

## 十、常见问题

### Q：PR 被 Request Changes 怎么办？

根据反馈修改后推送新提交到同一分支，PR 会自动更新，请审查者重新审查。

### Q：CI 失败但本地通过？

检查环境差异，例如引擎版本、依赖模块或平台差异，补充缺失依赖或修正平台相关问题。

### Q：如何解决合并冲突？

```bash
git checkout feature/your-branch
git fetch origin
git rebase origin/develop
# 解决冲突后
git add .
git rebase --continue
git push --force-with-lease origin feature/your-branch
```

### Q：如何撤销已合并的 PR？

使用 `git revert <merge-commit-sha> -m 1` 创建反向提交，再通过 PR 合并。

### Q：提交未签名或被规则集拦截？

确认本地已配置 GPG 签名与签核，并对相关提交补签后重新推送。具体配置可参考项目文档或向维护者询问。

---

## 十一、维护者

| 账号 | 职责 |
| --- | --- |
| [@bgzsdzzl](https://github.com/bgzsdzzl) | 核心维护者 |

---

## 许可

贡献代码即表示同意按 Apache License 2.0 发布。
