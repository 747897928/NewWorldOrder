import bpy, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
# 只加载增量建模工具，不重新执行整套房屋布局。
helper=(OUT/'extend_pavilion.py').read_text(encoding='utf-8').split('# 保持一层 U 型平面')[0]
exec(compile(helper,'pavilion_helpers','exec'))

for side in [-1,1]:
    box('Wing_Eave_'+str(side),(side*650,200,338),(140,2440,20),'Plaster','Roof')
    box('Wing_Eave_Soffit_'+str(side),(side*650,200,324),(138,2430,6),'Oak','Roof')
    box('Wing_Eave_ShadowGap_'+str(side),(side*581,200,321),(3,2440,4),'Metal','Roof',False)
box('Pavilion_Ceiling_Soffit',(0,-490,684),(1090,1000,8),'Oak','Roof')
box('Balcony_Front_Edge',(0,188,340),(1170,12,28),'Plaster','Gallery')
box('Balcony_ShadowGap',(0,184,321),(1150,3,4),'Metal','Gallery',False)

# 台阶独立方块的默认 0..1 UV 会把木纹压成碎块；改为米制连续投影。
stairs=bpy.data.meshes['SM_HM_GalleryStair']
uv=stairs.uv_layers.active or stairs.uv_layers.new(name='UVMap')
for poly in stairs.polygons:
    axis=max(range(3),key=lambda i:abs(poly.normal[i]));axes=[i for i in range(3) if i!=axis]
    for li in poly.loop_indices:
        co=stairs.vertices[stairs.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
new_meshes[stairs.name]=stairs
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
# 使用原 UCX 楼梯斜坡导出流程，仅导出这批新增尺寸和修过 UV 的楼梯。
exporter=(OUT/'extend_pavilion.py').read_text(encoding='utf-8').split("bpy.ops.object.select_all(action='DESELECT')\nfor name,mesh in new_meshes.items():")[1]
stairs_name=stairs.name
exec(compile("bpy.ops.object.select_all(action='DESELECT')\nfor name,mesh in new_meshes.items():"+exporter,'polish_exports','exec'))
