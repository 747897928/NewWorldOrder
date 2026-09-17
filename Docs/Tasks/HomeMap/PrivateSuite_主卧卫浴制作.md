# 主卧与卫浴制作

## 当前实现

- 本轮套件原在 HomeMap_Courtyard_BackUp 中验收；该关卡现已由用户重命名为 HomeMap_Courtyard。默认白天，U 型建筑保留，后续池长变更见泳池交接文档。
- 主卧床头调整朝西，床、床头柜和相应碰撞代理一起摆放。新增竖向木条、中央软包、细金属接缝和收拢帘，床前保留大面积活动空间。
- 卫浴原实心方块台盆、方块龙头与柜体占位 Actor 已由双台盆套件替代。盆腔有实际几何深度，柜体悬浮，台面、镜框、地面与背墙组合成一个静态套件；镜面继续使用原独立 Actor。
- 西侧补挂巾与收纳凳，小瓶、书籍、床、地毯、椅子继续引用已有 HomeMap 资源。
- Blender 源：SourceArt/HomeMap/HomeMap_Master.blend。本轮前可回退提交为 64cd6ff2，包含完整关卡和源文件。

## 资产与成本

- Architecture/Hub/SM_HM_BedroomHeadwall：7,332 三角面，5 个材质槽。
- Architecture/Hub/SM_HM_BedroomCurtain：2,732 三角面，2 个材质槽，复用为两侧收拢帘；不使用透明布料，不阻挡通行。
- Architecture/Hub/SM_HM_BathVanitySuite：6,144 三角面，5 个材质槽。包含地面和墙面，不能用一个大凸包配置碰撞；Static 实例采用 ComplexAsSimple。
- Architecture/Hub/SM_HM_SpaTowelStation：3,840 三角面，3 个材质槽，使用原木材、金属和织物。
- 床头与台盆套件开启 Nanite；窗帘和毛巾架保留普通低面数网格。全部不新增大纹理。
- 只新增 HM_Bedroom_Headwall_Wash 局部 RectLight：Stationary、无阴影、半径 300 cm。其他改动为原主卧和卫浴灯的强度与覆盖方向调整。

## 材质中的隐含配置

- MI_Suite_Linen 继承项目原 M_Fabric；只覆写实例，不修改母材质。
- 原椅垫 M_Armchair_seat 带有椅垫专用烘焙法线，不可直接铺到新床头大平面。实例 Normal Map 改用已有 T_Fabric_N，保留织物细节。
- M_Fabric 的 Dirt Intencity 名称容易误导：它接在 Lerp.A，Lerp.B=1，Alpha 为 Dirt Map。因此值 1 才能去除污渍对比；值 0 会出现最明显斑块。已通过原图输入连接确认，并在 configure_suite_linen.py 留下注释。
- 阅读桌原网格的实际高度约 40.91 cm，缩放 0.75 后为 30.68 cm；书籍底面配置为 30.8 cm，不能沿用早期脚本误写的 57 cm 桌高。

## 验证与后续

- Reports/private_suite_walk.json：使用既有角色和 CharacterMovement 实际连续走过卧室、衣帽通道、卫浴、回到床前；6 个路径点全部通过。
- 视觉记录：Reports/HomeMap_Bedroom_20260906.png、HomeMap_Bathroom_20260906.png。
- 衣帽、出征区后续收纳深化和 11 点通行记录已补充，见 JoineryLandscape_收纳与外围深化.md；Gallery 与康体区另见对应制作文档。这些局部成果不能代替全场景最终验收。
- 旧性能采样不包含本轮新增资产，完整场景阶段需要统一复测。
