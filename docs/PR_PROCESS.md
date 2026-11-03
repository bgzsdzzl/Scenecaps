# Pull Request 流程

本文档定义 Scenecaps 项目的 Pull Request 提交、审核与合并流程。

---

## 分支策略

| 分支 | 用途 | 保护 |
| ------ | ------ | ------ |
| `main` | 稳定发布分支 | 受保护，仅允许 PR 合并 |
| `develop` | 开发集成分支 | 受保护，仅允许 PR 合并 |
| `feature/*` | 新功能开发 | 临时，合并后删除 |
| `fix/*` | 缺陷修复 | 临时，合并后删除 |
| `docs/*` | 文档更新 | 临时，合并后删除 |
| `refactor/*` | 重构 | 临时，合并后删除 |
| `release/*` | 发布准备 | 临时，合并后删除 |
| `hotfix/*` | 紧急修复 | 从 main 切出，合并回 main/develop |

---

## 提交规范

### 分支命名

```text
feature/short-description    # 新功能

fix/short-description        # 缺陷修复
docs/short-description       # 文档更新
refactor/short-description   # 代码重构
test/short-description       # 测试相关
```

示例：

- `feature/biome-radial-distribution`
- `fix/variant-transition-crash`
- `docs/update-readme-installation`

---

### 提交信息规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/v1.0.0/)：

```text
<type>[optional scope]: <description>


[optional body]

[optional footer(s)]
```

**类型：**

- `feat` - 新功能
- `fix` - 缺陷修复
- `docs` - 文档更新
- `refactor` - 重构（无功能变更）
- `perf` - 性能优化
- `test` - 测试相关
- `chore` - 构建/工具/依赖更新
- `revert` - 回滚

**示例：**

```text
feat(editor): add per-object point lights to preview viewport


Disable default lighting and add per-object point lights
to avoid overbright preview.

Closes #123
```

```text
fix(runtime): disable default lighting in preview scene


Set CVS.bDefaultLighting = false to prevent overbright
preview caused by default sky/atmosphere lighting.

Fixes #456
```

---

## Pull Request 流程

### 1. 创建 PR 前

- [ ] 基于最新 `develop`（或 `main` 热修复）创建分支
- [ ] 同步上游最新代码：`git pull origin develop`
- [ ] 本地编译通过（Development + Shipping）
- [ ] 运行相关测试通过
- [ ] 代码符合 [CODING_STANDARDS.md](CODING_STANDARDS.md)

### 2. 创建 PR

- **Base 分支**：`develop`（功能/修复）或 `main`（hotfix）
- **标题**：遵循提交规范，如 `feat(editor): add per-object point lights to preview`
- **描述模板**：

```markdown
## 变更类型

- [ ] feat: 新功能

- [ ] fix: 缺陷修复
- [ ] refactor: 重构
- [ ] docs: 文档更新
- [ ] test: 测试相关
- [ ] chore: 构建/工具/依赖

## 变更描述
<!-- 简述变更内容与动机 -->


## 相关 Issue
Closes #123

Relates to #456

## 测试验证
- [ ] Editor (Development) 编译通过

- [ ] Game (Development) 编译通过
- [ ] Game (Shipping) 编译通过
- [ ] 相关功能手动测试通过
- [ ] 无新增编译警告

## 截图/视频 (如适用)
<!-- UI/渲染变更请附截图或视频 -->


## 影响范围
- [ ] Runtime 模块

- [ ] Editor 模块
- [ ] 公开 API 变更
- [ ] 配置/数据格式变更
- [ ] 文档需同步更新
```

### 3. 代码审核

#### 审核者职责

- 代码逻辑正确性

- 符合 [CODING_STANDARDS.md](CODING_STANDARDS.md)
- 无明显性能/内存问题
- 测试覆盖充分
- 文档同步更新

#### 审核流程

1. 自动 CI 检查通过

- [ ] Editor (Development) 编译

- [ ] Game (Development) 编译
- [ ] Game (Shipping) 编译
- [ ] 无新增编译警告

- [ ] 至少一位维护者 `Approve`
- [ ] 无 `Request Changes` 未解决
- [ ] 所有 `Conversation` 已解决

### 4. 合并策略

| 场景 | 合并方式 |
| ------ | ---------- |
| 常规功能/修复 | Squash and merge（压缩为单提交） |
| 多提交关联功能 | Rebase and merge（保留历史） |
| 热修复 | Squash and merge 到 main + cherry-pick 到 develop |

**合并前检查：**

- [ ] 所有 CI 通过
- [ ] 无 `Request Changes` 未解决
- [ ] 至少 1 个 `Approve`
- [ ] 分支已同步最新目标分支（`git rebase develop` 或 `git merge develop`）
- [ ] 无冲突

### 5. 合并后

- 删除源分支（GitHub 自动删除或手动删除）
- 关联 Issue 自动关闭（`Closes #123`）
- 如需回港：`git revert <merge-commit>`

---

## 发布流程

### 版本发布

1. **准备**：
   - 创建 `release/v1.x.x` 分支

   - 更新 `Scenecaps.uplugin` 版本号
   - 更新 `CHANGELOG.md`

2. **验证**：
   - 全配置构建通过

   - 核心功能冒烟测试
   - 打包 Shipping 配置验证

3. **发布**：
   - `git tag v1.x.x`

   - `git push origin v1.x.x`
   - GitHub Actions 自动创建 Release、上传产物
   - 更新 `CHANGELOG.md` 最终版本

### 热修复

1. 从 `main` 切出 `hotfix/v1.x.y`
2. 修复并验证
3. PR 合并到 `main`（Squash）
4. `git tag v1.x.y` 推送
5. Cherry-pick 到 `develop`

---

## 回滚流程

如发现严重回归：

```bash
# 回滚合并提交

git revert <merge-commit-sha> -m 1

git push origin main

# 或回滚到指定标签
git checkout v1.2.3

git push origin main --force-with-lease
```

---

## 常见问题

### Q: PR 被 Request Changes 怎么办？

A: 根据反馈修改，推送新提交到同一分支，PR 自动更新，请审核者再次审核。

### Q: CI 失败但本地通过？

A: 检查环境差异（UE 版本、插件版本、依赖模块），补充缺失依赖或修复平台差异。

### Q: 如何解决合并冲突？

```bash

git checkout feature/your-branch

git fetch origin
git rebase origin/develop  # 或 merge
# 解决冲突

git add .

git rebase --continue
git push -f origin feature/your-branch
```

### Q: 如何撤销已合并的 PR？

A: `git revert <merge-commit-sha> -m 1` 创建反向提交，再次 PR 合并。

---

## 维护者名单

| 姓名 | GitHub | 职责 |
|------|--------|------|
| @bgzsdzzl | [bgzsdzzl](https://github.com/bgzsdzzl) | 核心维护者 |
| ... | ... | ... |

---

## 许可证

贡献代码即同意按 Apache License 2.0 许可。
