# Contributing Workflow

## 分支策略
- `feature/<area>-<summary>`：新功能或大型改造。
- `fix/<area>-<issue>`：缺陷修复或回归处理。

## 提交信息
- 使用祈使句，格式如 `feat(weapons): add spread recovery tuning`。
- 单个提交聚焦一个逻辑变更，方便回溯。

## Pull Request 审核
- 必填：问题背景、复现步骤、平台影响、截图/视频或日志。
- 标明需要同步更新的文档（`AGENTS.md`、`Docs/*`）和自动化测试结果。

## 测试与自动化
- 测试代码放在 `Source/<Module>/Tests`。
- 自动化命令：  
  `UnrealEditor.exe NewWorldOrder.uproject -unattended -nop4 -NoSound -NullRHI -ExecCmds="Automation RunTests NewWorldOrder;Quit"`
- 若添加临时验证脚本，请在合并前移除或迁移到工具目录。
