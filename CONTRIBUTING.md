# 贡献指南

感谢您对 Scenecaps 的关注与贡献！

---

## 快速开始

1. Fork 仓库
2. 创建特性分支：`git checkout -b feature/your-feature-name`
3. 提交更改
4. 确保构建通过
5. 提交 Pull Request

---

## 核心文档

- [编码规范](docs/CODING_STANDARDS.md)
- [测试要求](docs/TESTING.md)
- [Pull Request 流程](docs/PR_PROCESS.md)
- [生成性 AI 使用方针](docs/AI_USAGE_POLICY.md)

---

## 快速检查清单

提交 PR 前请确认：

- [ ] 代码符合 UE 编码规范
- [ ] 无编译警告（Development + Shipping）
- [ ] 无内存泄漏 / GC 问题
- [ ] 编辑器代码用 `WITH_EDITOR` / `WITH_EDITORONLY_DATA` 包裹
- [ ] 公开 API 有 `UFUNCTION(BlueprintCallable)` 等标记
- [ ] 文档已同步更新

---

## 问题反馈

- Bug 与功能请求使用 GitHub Issues
- 包含：UE 版本、插件版本、复现步骤
- 崩溃请附调用栈与日志片段

---

## 行为准则

请遵守 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)。

---

## 许可证

贡献代码即同意按 Apache License 2.0 许可。