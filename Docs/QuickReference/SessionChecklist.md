# Session Checklist (每次会话开工前确认)

1. 先判断当前环境：Windows / WSL(Linux) / macOS；命令路径用相对项目根目录的写法，不写盘符绝对路径。
2. 读 AGENTS.md 与 Docs/DevelopmentNotes/MCP_踩坑记录.md。
3. MCP：确认 8000 属于目标项目实例；只有 localhost 请求被系统代理劫持时才加 --noproxy "*"；ping 通但 execute_python_code 挂时先查编辑器是否冻结；/mcp 404 时执行 ModelContextProtocol.StartServer 8000 切回。
4. C++：小改动可先 Live Coding；一旦出现 patch DLL 崩溃或编辑器冻结，清理 patch 残留并改冷编译（沙箱内冷编译需 full-access）。
5. 删除：单文件验证路径后可走回收站；多个文件或非空目录停下等用户确认。
6. 修改完成后：中文文档用安全 UTF-8；git add 具体路径 → commit → push。
7. 跨会话或核心系统任务：在 Docs/Tasks/<TaskName>/ 建任务包（STATUS + Context/Requirements/Implementation）。
