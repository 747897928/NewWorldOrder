import bpy, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
helper=(OUT/'extend_pavilion.py').read_text(encoding='utf-8').split('# 保持一层 U 型平面')[0]
exec(compile(helper,'site_helpers','exec'))
# 室内/庭院标高为 0，外围场地为 -150 cm。补实体基座，避免从外侧看见悬空楼板。
for name,loc,dims in [
    ('Foundation_Public',(0,-500,-90),(2800,1000,120)),
    ('Foundation_Private',(-1050,700,-90),(700,1400,120)),
    ('Foundation_Wellness',(1050,700,-90),(700,1400,120)),
    ('Foundation_DeckLiving',(0,125,-90),(1400,250,120)),
    ('Foundation_DeckWest',(-570,825,-90),(260,1150,120)),
    ('Foundation_DeckEast',(570,825,-90),(260,1150,120)),
    ('Foundation_DeckNorth',(0,1390,-90),(1400,180,120)),
    ('Foundation_Arrival',(0,-1280,-95),(1200,560,110)),
    ('Foundation_RearGarden',(0,1615,-90),(3450,270,120)),
    ('RearGarden_Terrace',(0,1615,-15),(3450,270,30))]:box(name,loc,dims,'Stone','Landscape')
for x in [-1730,1730]:box('Garden_Wall_'+str(x),(x,300,15),(25,3000,330),'Stone','Landscape')
box('Garden_North_Wall',(0,1750,25),(3480,25,350),'Stone','Landscape')

# 有限范围的低密度背景地形，建筑附近保持平整；不用高分辨率巨型 Landscape。
name='SM_HM_SiteTerrain';mesh=bpy.data.meshes.get(name)
if not mesh:
    vertices=[];faces=[]
    for j in range(51):
        y=-50+j*2
        for i in range(51):
            x=-50+i*2;outside=max(abs(x)-20,abs(y)-23,0);t=min(outside/20,1);s=t*t*(3-2*t)
            z=-1.5+s*(1.5+2*math.sin(.08*x+.11*y)**2+.7*math.cos(.17*x-.09*y)**2)
            vertices.append((x,y,z))
    for j in range(50):
        for i in range(50):
            a=j*51+i;faces.extend([(a,a+1,a+52),(a,a+52,a+51)])
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
    uv=mesh.uv_layers.new(name='UVMap')
    for poly in mesh.polygons:
        poly.use_smooth=True
        for li in poly.loop_indices:
            co=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(co.x,co.y)
    mesh.materials.append(bpy.data.materials['Preview_Garden'])
ob=bpy.data.objects.get('Site_Ground');ob.data=mesh;ob.location=(0,0,0)
r=by_name['Site_Ground'];r.update(mesh=name,loc=[0,0,0],dims=[10000,10000,450])
new_meshes[name]=mesh
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
exporter=(OUT/'extend_pavilion.py').read_text(encoding='utf-8').split("bpy.ops.object.select_all(action='DESELECT')\nfor name,mesh in new_meshes.items():")[1]
stairs_name='SM_HM_GalleryStair'
exec(compile("bpy.ops.object.select_all(action='DESELECT')\nfor name,mesh in new_meshes.items():"+exporter,'site_exports','exec'))
