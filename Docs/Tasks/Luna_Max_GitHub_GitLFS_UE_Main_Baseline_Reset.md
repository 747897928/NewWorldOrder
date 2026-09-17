# Luna Max 任务：将 Unreal Engine 项目的 Git / GitHub / Git LFS 以当前 Main 重新建立基线

## 1. 任务背景

这是一个使用 Git 管理的 Unreal Engine 游戏项目，远程仓库托管在 GitHub，并且已经使用 Git LFS 管理大型二进制资产。

项目开发过程中曾频繁整理 Unreal Engine `Content` 目录，例如移动大量 `.uasset`、`.umap` 以及其他资源到新的文件夹路径，同时资源本身也经历过修改、重新保存、重新导入等操作。长期积累后，本地 `.git` 目录已经膨胀到约 12 GB。

本次不需要保留过去这些历史作为日常开发历史。目标是：

**以执行任务时 Git `Main` 分支当前已提交的内容为唯一基准，把它视为项目新的历史起点。**

需要同时处理：

1. 本地 Git 历史。
2. 本地 Git LFS 缓存。
3. GitHub 远程仓库历史。
4. GitHub 远程 Git LFS 历史对象。
5. 完成后验证新的仓库能够独立 clone、拉取 LFS 并正常作为 Unreal Engine 项目继续开发。

不要为了保留旧提交而使用复杂的历史过滤。旧历史可以归档，但新的正式仓库只需要从当前 Main 开始。

---

## 2. 最终目标

完成后，正式 GitHub 仓库应满足：

```text
Initial baseline from Main - <date>
        |
        +-- 以后新的开发提交
```

新的第一个 commit 必须：

- 没有 parent，是一个真正的 root commit。
- tree 与执行本任务前 `Main` HEAD 的 tree 完全一致。
- 不因为重建历史而修改、遗漏或重新生成 Unreal Engine 项目内容。
- 保留正确的 `.gitignore`、`.gitattributes` 和当前 Git LFS tracking 规则。
- 当前需要由 Git LFS 管理的资产仍然由 Git LFS 管理。

最终还要做到：

- 新 GitHub 仓库中不再保留旧 Git commit 历史。
- 新 GitHub 仓库只关联当前基线及之后真正需要的 Git LFS 对象。
- 从全新的空目录 clone 后，可以完整恢复项目。
- 本地正式工作副本不再携带旧 `.git` / `.git/lfs/objects` 历史垃圾。

---

## 3. 重要原则

### 3.1 当前 Main 的内容不可被清理流程改变

本任务是“重新建立 Git 历史基线”，不是重构项目内容。

不要在没有必要的情况下：

- 删除资产。
- 移动资产。
- 重命名资产。
- 批量重新保存 `.uasset` / `.umap`。
- 重新导入资产。
- 修改蓝图、C++、Config 或插件代码。
- 为了缩小 Git 而改变实际项目文件。

新的 root commit 应直接复用当前 Main 的 Git tree。

### 3.2 不要使用 orphan branch 重新 add 整个项目

不要以“创建 orphan branch，然后删除/重新添加全部文件”的方式处理这个 Unreal Engine 大型项目。

优先使用：

```bash
git commit-tree <MainTree> -m "Initial baseline from Main - <date>"
```

`git commit-tree` 在不提供 `-p` parent 时会生成没有父提交的 root commit，并且可以直接引用当前 Main 已存在的 tree，因此无需重新扫描和重新添加几十 GB 的项目文件。

### 3.3 GitHub LFS 必须与普通 Git 历史分开理解

仅仅 force push 一个新的 Main root commit，不能让 GitHub 自动删除过去已经上传的 Git LFS 对象。

GitHub 官方规则是：从 Git 历史中去掉 LFS 文件以后，旧 LFS 对象仍可能继续保存在远程并计入 Git LFS storage。若希望真正清除仓库关联的旧 Git LFS 对象，官方方案是删除并重新创建仓库；如果不能删除仓库，则需要联系 GitHub Support。

因此，本任务如果要实现“GitHub LFS 也从当前 Main 重新计算”，必须最终让**新的 GitHub repository identity**只接收当前基线所引用的 LFS 对象，而不能只重写旧仓库的 Main。

---

## 4. 安全约束

这是具有破坏性的仓库维护任务。必须遵守以下顺序。

### 禁止事项

在完成备份和验证之前，不允许：

- 删除本地原仓库。
- 删除 GitHub 原仓库。
- 删除任何无法恢复的本地资产。
- 清理原仓库 `.git/lfs/objects`。
- 清理 reflog 后再去做唯一备份。
- 删除远程旧仓库后才开始检查 LFS 是否完整。
- 假设 GitHub 仓库没有 Issues、Releases、Actions、Secrets、Deploy Keys、Webhooks、Branch Rules 等数据。

### 必须保留的恢复点

至少建立两个恢复点：

1. **项目文件完整备份**：原项目目录或至少完整工作树复制到仓库外的安全位置。
2. **旧 Git 历史备份**：保留原 `.git` 的完整副本，或者保留整个旧项目目录，直到新仓库通过最终验证。

Git LFS 项目不要只依赖 `git bundle` 作为唯一备份，因为 bundle 保存的是 Git 对象和 LFS pointer，不等同于完整备份所有 LFS 二进制内容。

---

## 5. 第一阶段：自动检查当前仓库

请先自行定位项目 Git root，不要假设固定盘符和固定路径。

记录以下信息到执行日志中：

```bash
git rev-parse --show-toplevel
git status --short --branch
git remote -v
git branch -vv
git tag --list
git rev-parse HEAD
git rev-parse HEAD^{tree}
git count-objects -vH
git lfs version
git lfs env
git lfs status
git lfs track
git lfs ls-files
```

同时检查：

- 当前实际默认开发分支的准确名称和大小写，不要武断假设一定叫 `main` 或 `Main`。
- `origin` 实际 GitHub repository URL。
- 是否存在其他远程 branch/tag 仍引用旧历史。
- `.gitattributes`。
- `.gitignore`。
- `.git/objects` 大小。
- `.git/lfs/objects` 大小。
- 当前 Main HEAD 是否干净。
- 当前 Main 中所有 LFS pointer 是否能在本地找到对应 LFS object。

执行：

```bash
git lfs fsck
```

如果 `git lfs fsck` 有错误，先解决完整性问题，不要继续破坏远程仓库。

### 工作树有未提交内容时

本任务的基线定义为“任务开始时 Main 的已提交 HEAD”。

如果工作树存在未提交修改：

- 不允许删除或覆盖这些修改。
- 在仓库外额外备份这些修改涉及的文件。
- 可生成 patch/状态清单供恢复。
- 新 root commit 默认仍以当时 Main HEAD 的 tree 为基准，而不是擅自把未知未提交内容混入新的历史起点。

---

## 6. 第二阶段：检查 Unreal Engine Git / LFS 配置

不要随意扩大现有 LFS 规则，但必须确认 UE 核心二进制资产没有错误地作为普通 Git blob 管理。

重点检查至少：

```text
*.uasset
*.umap
```

它们在当前仓库策略中应正常由 Git LFS 管理。

同时检查项目现有 `.gitattributes` 中对 FBX、音频、贴图源文件、PSD、EXR、视频或其他大型源资产的规则，但不要根据通用模板盲目更改项目原有策略。

如果发现当前 Main 中存在本应由现有 `.gitattributes` LFS 规则管理、却仍被普通 Git 直接存储的大型文件，先记录问题。只有在能够保证不改变最终实际文件内容、并且确认属于配置错误时，才修正当前基线；否则保持 Main tree 不变并在报告中说明。

检查 `.gitignore` 是否至少正确排除了 Unreal Engine 常见生成目录，例如：

```text
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
```

但不要因为本任务擅自删除项目中有意纳入版本控制的特殊内容。

---

## 7. 第三阶段：建立旧仓库外部备份

在任何 history rewrite 或远程操作之前：

1. 关闭 Unreal Editor 和可能持续写入项目文件的工具。
2. 将当前项目完整复制到仓库之外的备份位置。
3. 记录备份路径。
4. 确认备份中包含完整工作树和 `.git`。
5. 对关键文件数量/大小做基本核对。

推荐备份命名：

```text
<ProjectName>_Before_Git_Baseline_Reset_<YYYY-MM-DD>
```

旧备份在新仓库最终验证完成前不可删除。

---

## 8. 第四阶段：从当前 Main tree 创建新的 root commit

先保存旧 Main 信息：

```bash
OLD_MAIN_COMMIT=$(git rev-parse HEAD)
OLD_MAIN_TREE=$(git rev-parse HEAD^{tree})
```

创建没有 parent 的新 commit：

```bash
NEWROOT=$(git commit-tree "$OLD_MAIN_TREE" -m "Initial baseline from Main - <YYYY-MM-DD>")
```

验证：

```bash
git cat-file -p "$NEWROOT"
git diff "$OLD_MAIN_COMMIT" "$NEWROOT"
```

必须满足：

- `git cat-file -p` 中有 `tree`，但没有 `parent` 行。
- `git diff OLD_MAIN_COMMIT NEWROOT` 没有任何输出。
- 新 root 的 tree hash 与旧 Main tree hash 完全相同：

```bash
test "$(git rev-parse "$NEWROOT^{tree}")" = "$OLD_MAIN_TREE"
```

只有这些验证全部通过后，才允许把正式本地 Main 引用切到新 root commit。

在切换前另外创建一个**仅存在于外部备份中的旧 commit 记录文件**，记录：

```text
OLD_MAIN_COMMIT=<hash>
OLD_MAIN_TREE=<hash>
NEWROOT=<hash>
ORIGIN=<url>
DATE=<date>
```

不要为了保留旧历史而在最终 GitHub 新仓库创建 `old-main`、`backup-main`、旧 tag 等引用。

然后让本地正式 Main 指向 `NEWROOT`。

具体 ref 更新方式由你根据仓库实际分支名选择，必须确保不会改变工作树内容。

完成后验证：

```bash
git status
git log --oneline --decorate --graph --all
git rev-parse HEAD^{tree}
git lfs status
git lfs fsck
```

正式 Main 的 tree 必须仍等于 `OLD_MAIN_TREE`。

---

## 9. 第五阶段：GitHub 远程迁移策略

### 推荐策略：先保留旧 GitHub 仓库，再建立新的正式仓库

不要先删除唯一的 GitHub 远程仓库。

更安全的做法：

1. 识别当前 GitHub `owner/repository`。
2. 盘点仓库级数据。
3. 将旧 repository 重命名为临时 legacy 名称，例如：

```text
<ProjectName>-legacy-before-baseline-reset-<date>
```

4. 用原来的正式 repository 名称创建一个新的空 GitHub repository。
5. 把新的 root Main 推到这个新 repository。
6. 上传/确认当前 Main 所需的 Git LFS 对象。
7. 从新 repository fresh clone 到新目录。
8. 完整验证。
9. 只有新仓库验证成功后，才删除 legacy repository。

这样在验证新仓库前，旧 GitHub 仓库仍然存在，可以回滚。

### 为什么最终仍需要删除 legacy repository

GitHub 官方说明：Git LFS 对象即使从 Git 历史中移除，也仍可能保留在原 repository 的远程 LFS storage 中并计入配额。删除原 repository 才能按官方方案清除该 repository 关联的旧 LFS 对象。

因此：

- 新 repository：只上传当前 baseline 需要的 LFS 对象。
- legacy repository：保留旧 LFS 对象用于暂时回滚。
- 新仓库确认无误后删除 legacy repository：最终摆脱旧 LFS 历史。

---

## 10. 删除/重建 GitHub 仓库前必须盘点的内容

在重命名或删除原 repository 之前，检查并记录是否存在：

- Issues。
- Pull Requests。
- Releases / release assets。
- Wiki。
- Discussions。
- Stars / Watchers / Fork 关系。
- GitHub Actions workflows。
- Actions repository secrets / variables / environments。
- Deploy Keys。
- Webhooks。
- Branch protection / Rulesets。
- GitHub Pages。
- Repository permissions / collaborators / teams。
- Repository topics / description / visibility。
- GitHub Apps 或其他 repository-scoped integration。

能够导出的内容应先导出或记录。

**Actions secret 的明文通常无法从 GitHub 重新读取。** 如果原 repository 依赖无法恢复的 secret、第三方集成或其他不可复制配置，不要静默删除它。先在执行报告中明确指出阻塞项，并保留 legacy repository。

如果仓库只是个人开发代码仓库，没有需要保留的 GitHub metadata，则可以继续完成删除 legacy repository 的步骤。

---

## 11. 第六阶段：向新的 GitHub repository 推送 baseline

新的 GitHub repository 必须是空仓库，不要自动创建 README、License 或新的 `.gitignore` commit。

将本地 `origin` 明确设置为新的正式 repository URL，并确认：

```bash
git remote -v
```

推送新的 Main：

```bash
git push -u origin <MainBranch>
```

Git LFS pre-push hook 应上传这个 Main 当前引用且新 repository 尚不存在的 LFS objects。

然后显式确认：

```bash
git lfs push --all origin <MainBranch>
git lfs fsck
```

注意：新的正式仓库不要推送旧 branch、旧 tag 或旧 commit。

不要执行会把 `--all` Git refs 整体推送到新仓库的操作。

`git lfs push --all origin <MainBranch>` 中的 `--all` 只用于指定分支可达的 LFS 对象检查/上传，不等于把所有 Git refs 推过去。

---

## 12. 第七阶段：fresh clone 验证

这是本任务成功与否的最终判定，不允许省略。

在一个全新的目录 clone 新 GitHub repository：

```bash
git clone <new-repository-url> <ProjectName>_CleanClone
cd <ProjectName>_CleanClone
git lfs pull
```

验证：

```bash
git status
git log --oneline --decorate --graph --all
git rev-parse HEAD^{tree}
git lfs status
git lfs fsck
git lfs ls-files
git count-objects -vH
```

必须满足：

1. clone 成功。
2. Git LFS 下载成功，无 pointer 缺失。
3. `git lfs fsck` 无错误。
4. fresh clone 的 Main tree hash 与原任务开始时 `OLD_MAIN_TREE` 完全一致。
5. Git history 只从新的 root baseline 开始。
6. 没有旧 branch/tag 意外被带入。
7. 工作树 clean。

然后验证 Unreal Engine 项目：

- `.uproject` 存在且正确。
- `Content` 完整。
- `Config` 完整。
- `Source` 完整（如果项目有 C++）。
- `Plugins` 中应版本控制的插件完整。
- 关键 `.umap` / `.uasset` 不是残留的 LFS pointer 文本。
- 可以正常生成项目文件/编译（若项目需要）。
- Unreal Editor 可以打开项目。
- 项目主要地图能够加载。
- Blueprint / C++ / 插件不存在由 Git/LFS 重建造成的缺失。

不要因为 UE 会自然生成 `Intermediate`、`Saved`、`DerivedDataCache` 等目录而认为 Git 状态异常；这些目录应由 `.gitignore` 处理。

---

## 13. 第八阶段：删除 GitHub legacy repository

只有第 12 节 fresh clone 与 UE 验证完全成功后，才允许处理旧 GitHub repository。

如果第 10 节盘点确认没有必须保留、且无法迁移的 GitHub repository metadata：

1. 删除 legacy repository。
2. 确认新的正式 repository 仍然存在且 URL 正确。
3. 再次检查新的 remote 和 clone。

这样旧 repository 关联的历史 Git LFS objects 才不会继续作为正式仓库的一部分保留。

GitHub Git LFS billing/storage 页面可能不会在当前计费月立即把已经发生的 usage 重算为零；GitHub 官方说明 storage usage 的月度计费显示存在按月结算规则。因此不要以“当天账单数字是否立刻下降”作为唯一成功标准。

---

## 14. 第九阶段：本地正式工作副本处理

最安全的做法不是继续在原 12 GB `.git` 仓库上做激进 GC，而是：

**直接把通过验证的 fresh clone 作为今后的正式开发工作副本。**

原项目目录保留为临时归档，确认一段时间没有问题后再人工决定是否删除。

如果确实还需要清理原本地仓库，可在所有验证完成以后执行：

```bash
git reflog expire --expire=now --all
git gc --prune=now
git lfs prune
```

但这不是必须步骤，因为 fresh clone 本身就是最干净、最容易验证的新本地 Git 数据库。

不要在备份尚未确认可用之前执行这些不可逆清理。

---

## 15. 完成后需要给我的执行报告

任务结束后，不要只说“完成了”。请给出结构化报告，至少包含：

### 原仓库

```text
Repository root:
GitHub repository:
Main branch:
Old Main commit:
Old Main tree:
Original .git size:
Original .git/objects size:
Original .git/lfs/objects size:
Original Git LFS file count:
```

### 新基线

```text
New root commit:
New root tree:
Has parent: no
Old tree == New tree: yes/no
```

### 新 GitHub 仓库

```text
New repository URL:
Main push: success/failure
LFS upload: success/failure
Legacy repository deleted: yes/no
If no, reason:
```

### fresh clone

```text
Clone path:
Clone success: yes/no
LFS fsck: pass/fail
Tree hash matches original Main: yes/no
Git working tree clean: yes/no
Unreal project opens: yes/no
Main map loads: yes/no/not tested
```

### 容量对比

```text
Old .git size:
New clean clone .git size:
Old ordinary Git object size:
New ordinary Git object size:
Old local LFS cache size:
New local LFS cache size:
```

并列出任何仍需人工处理的项目。

---

## 16. 成功标准

只有同时满足以下条件才能宣布完成：

- 当前 Main 的项目内容没有被历史清理过程改变。
- 新 root commit 的 tree 与旧 Main HEAD tree 完全相同。
- 新 root commit 没有 parent。
- 新 GitHub repository 不包含旧 Git history。
- 当前需要的 Git LFS 对象全部可以从新 GitHub repository 恢复。
- fresh clone 的 tree hash 与旧 Main tree 相同。
- `git lfs fsck` 通过。
- Unreal Engine 项目可以从 fresh clone 正常使用。
- 旧 GitHub repository 只有在上述验证通过后才被删除。
- 最终正式本地工作副本是 clean clone，不再背负过去约 12 GB 的旧 Git/LFS 历史缓存。

---

## 17. 官方行为依据

本方案依赖以下 Git / GitHub 官方行为：

1. `git commit-tree` 可以从一个现有 tree 创建 commit；不指定 parent 时可以生成 initial/root commit。
   - https://git-scm.com/docs/git-commit-tree

2. Git LFS 在普通 Git repository 中保存 pointer，实际大文件由 LFS 单独保存。
   - https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage

3. GitHub 官方说明，从 Git 历史删除 LFS 文件后，LFS objects 仍会存在于远程存储并继续计入 Git LFS storage；若要从 repository 中删除这些 LFS objects，官方方案是删除并重新创建 repository，无法删除时联系 GitHub Support。
   - https://docs.github.com/en/repositories/working-with-files/managing-large-files/removing-files-from-git-large-file-storage

4. GitHub Git LFS storage 会按 repository 关联的 LFS objects 计算；一个 LFS 文件上传新版本时，新版本整个文件大小会增加 storage usage。
   - https://docs.github.com/en/billing/concepts/product-billing/git-lfs

---

## 18. 给 Luna Max 的最终执行要求

你可以访问项目磁盘、Git、本机 Git LFS 和项目上下文，因此请自己完成路径发现、仓库检查、备份、Git 操作、GitHub 状态盘点与验证，不要把可自行检查的问题反问给我。

本任务宁可多做验证，也不要为了省步骤冒险删除唯一恢复点。

优先保证：

```text
数据完整性 > 可回滚性 > 新仓库正确性 > 减少磁盘占用
```

不要把“`.git` 变小了”当作唯一目标。真正目标是：

**以当前 Main 的确切项目状态建立一个新的、可独立恢复的、干净的 GitHub + Git LFS 历史起点。**
