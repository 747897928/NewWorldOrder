# HomeMap 庭院住宅

- 需求来源：2026-09-05 用户完整环境制作委托；本阶段只制作环境，复用现有角色、相机、Experience、交互。
- 当前制作基准：`/Game/Environment/HomeMap/Maps/HomeMap_Courtyard`。2026-09-06 用户将已验收并继续细化的 BackUp 重命名为正式名称；旧正式关卡保留为 `HomeMap_Courtyard_demise`。已重新加载并核对主卧、卫浴、池底和默认白天，后续不要继续打开旧 BackUp 路径。
- 主源文件：`SourceArt/HomeMap/HomeMap_Master.blend`；建筑导出位于同目录 `Exports`。

## 方案比较与选择

- A：双层住宅，约 460 平方米。占地紧凑，楼梯与垂直视线好看，但日常 Hub 动线较长，相机验证成本较高。
- B：单层 L 形住宅，约 430 平方米。公共区域直接面泳池，但私人区与康体区容易共用长走廊。
- C：U 形住宅，一层约 476 平方米。选定基底为 28×10 米公共主翼，两侧各 7×14 米，围合泳池庭院。按用户后续视觉目标，中央客厅增高到约 6.9 米净高，增加后侧 Gallery、西侧廊和朝泳池的阳台；两翼净高仍为 3.3 米。一层平面和泳池主体保持原位。

## 空间和动线

- 南侧门厅进入客厅，视线朝北穿过大面积玻璃和开放出口，到达泳池庭院。
- 西南是出征准备、收藏展示和既有副本门；东南是厨房、餐厅。实体参考板来自环境身份设计，并非已确认的任务系统需求；用户明确保留已制作的陈设，不接入任务、奖励或独立服务器逻辑。
- 西翼是卧室、衣帽、梳妆和卫浴；东翼是健身、瑜伽和康体休息。
- 主要开口 180～240 厘米，庭院两侧保留连续通行带，种植带靠建筑侧布置。泳池通过宽踏步进入；Luna 已接通游泳，当前实现与水体范围见 Swimming_Implementation_实现说明.md。2026-09-07 增加阳台直线跳台与 3.8 m 深水段，具体通行验收状态见 STATUS。
- Gallery 楼面标高 352 厘米；180 厘米宽直跑楼梯沿客厅东侧上行，20 级，每级 17.6 厘米。西侧廊可到达观景阳台；中央客厅保留挑空。二层只布置收藏、阅读和休息，一层保留衣柜、副本入口与全部高频功能。
- 楼梯采用 BFEU 试点验证的本地网格和 Actor Transform，中心 X=360；2026-09-08 根据实际试玩反馈撤掉入口叠放玻璃、落地细框、椅子和矮灯，一层改为连续开放通道，保留原白色结构框。入户新增默认向外打开的双扇门。现有角色从门外连续上楼、通过 Gallery 并跳入泳池通过；不要重跑旧门框脚本或护栏旋转补偿。
- 默认时间为白天。入户公共区的 `HM_DayNight_Control` 使用木饰面控制台模型，复用既有交互操作循环白天、黄昏、夜晚，不增加按键。

## 实施约束

- 新地图 WorldSettings 的 `AWorldSettings::DefaultGameMode` 选择已有 `BP_ShootGameMode`。该蓝图继承 `AShootGameModeBase`，使用父类的 `ExperienceDefinition` 指向 Home Experience。
- 原 ArchViz 资产只引用，不覆盖其材质和碰撞。按用户进一步明确的目录规范，新建筑放 `/Game/Environment/HomeMap/Architecture/Hub`，场景专用材质放 `/Game/Environment/HomeMap/Materials/Hub`，家具继续直接引用现有 Furniture、Props 和 Materials。
- 不创建 PlanarReflection，先用场景 PostProcessVolume 配置 Lumen、曝光和适度调色；不擅自改项目全局渲染设置。
- 实际验证和局限写入 `STATUS_制作状态.md`；未获得目标显卡实测不得宣称达到 60 FPS。
