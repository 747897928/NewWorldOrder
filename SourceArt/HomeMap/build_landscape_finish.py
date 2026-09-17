"""外围山脊与屋顶休息区；不覆盖场内地形或游泳池。"""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeLandscapeFinish.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
palette={'Wood':('M_Wall_Wood',(.22,.13,.075,1)), 'Metal':('M_Metall_Black',(.025,.028,.032,1)), 'Stone':('Hub/M_Limestone',(.42,.39,.33,1)), 'Bronze':('Hub/M_Bronze',(.25,.17,.08,1)), 'Glow':('Hub/M_Warm_Light',(1,.7,.38,1))}
palette['Glass']=('Hub/M_Architectural_Glass',(.2,.3,.35,.12))
palette['Linen']=('Hub/MI_Suite_Linen',(.5,.47,.4,1))
palette['Terrain']=('Hub/M_EstateTerrain',(.12,.16,.07,1))
mats={}
for n,(path,col) in palette.items():
    m=bpy.data.materials.get('Closure_'+n) or bpy.data.materials.new('Closure_'+n);m.diffuse_color=col;mats[n]=m
parts=[];manifest=[]
def p(v):return (v[0]/100,-v[1]/100,v[2]/100)
def box(loc,dims,mat,bevel=.4):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p(loc));o=bpy.context.object;o.dimensions=[v/100 for v in dims];bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Finished edges','BEVEL');mod.width=bevel/100;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(mats[mat]);parts.append(o);return o
def export(name,collision):
    global parts
    old=bpy.data.objects.get(name)
    if old:bpy.data.objects.remove(old,do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;o.data.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    # 开放地形没有封闭体积，重算法线可能整体朝下；必须验证朝上，不能用双面材质遮掩。
    if name=='HM_EstateBackdrop' and sum(f.normal.z for f in bm.faces)<0:bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
    bm.to_mesh(o.data);bm.free()
    uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
    for f in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(f.normal[j]))]
        for li in f.loop_indices:
            co=o.data.vertices[o.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
    o.data.update();bpy.context.view_layer.update()
    bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh'/('SM_'+name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
    o.data.calc_loop_triangles()
    manifest.append({'mesh':'SM_'+name,'triangles':len(o.data.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/'+palette[m.name.removeprefix('Closure_')][0] for m in o.data.materials],'nanite':False,'collision':collision,'rebuild_material_slots':True,'instances':[{'label':name,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
    parts=[]

# 用原场地边界高度生成连续外环；保留中心 100×100 m 的现有房屋、池底和场地。
from mathutils.bvhtree import BVHTree
from mathutils import noise
terrain=bpy.data.objects['Site_Ground'];tree=BVHTree.FromObject(terrain,bpy.context.evaluated_depsgraph_get())
perimeter=[]
for side in range(4):
    for i in range(50):
        t=-50+2*i
        perimeter.append([(t,-50),(50,t),(-t,50),(-50,-t)][side])
inner=[]
for x,y in perimeter:
    hit=tree.ray_cast(Vector((x*.9998,-y*.9998,2000)),Vector((0,0,-1)))
    assert hit[0] is not None,(x,y)
    inner.append(hit[0].z)

def hill(x,y,cx,cy,height,wx,wy):return height*math.exp(-(((x-cx)/wx)**2+((y-cy)/wy)**2))
def height(x,y):
    d=max(abs(x),abs(y))
    value=2+hill(x,y,-130,90,38,65,85)+hill(x,y,150,140,42,80,65)+hill(x,y,90,-150,27,65,75)+hill(x,y,-200,-160,55,90,80)
    value+=hill(x,y,-520,360,180,230,310)+hill(x,y,480,510,160,220,290)+hill(x,y,160,-650,220,420,230)+hill(x,y,-750,-430,180,290,220)
    value+=hill(x,y,100,1400,340,650,400)+hill(x,y,-1350,300,300,400,800)+hill(x,y,1400,-350,320,480,690)
    amp=min(12,max(0,d-50)*.05)/(1+(d/280)**2)
    value+=amp*(.5*math.sin(x*.037+math.sin(y*.021))+.25*math.sin(y*.071+x*.043)+.12*math.cos(x*.11-y*.08))
    # fBm 打破规则圆丘轮廓，远山降低高度，让屋顶仍保有开阔天空。
    detail=noise.fractal(Vector((x/95,y/95,1.7)),.85,2.,4.)
    broad=noise.fractal(Vector((x/310,y/310,4.2)),.85,2.,4.)
    return value*.62+detail*min(9,max(0,d-50)*.12)+broad*min(34,max(0,d-150)*.09)
radii=[50*(2450/50)**(i/64) for i in range(65)]
v=[];f=[]
for r in radii:
    blend=min(1,max(0,(r-50)/40));blend=blend*blend*(3-2*blend)
    for j,(a,b) in enumerate(perimeter):
        x,y=a*r/50,b*r/50;z=inner[j]*(1-blend)+height(x,y)*blend
        v.append((x,-y,z))
for ring in range(len(radii)-1):
    for i in range(200):
        a=ring*200+i;b=ring*200+(i+1)%200;f.append((a,b,b+200,a+200))
me=bpy.data.meshes.new('Estate continuous mountain rings');me.from_pydata(v,[],f);me.update();me.materials.append(mats['Terrain'])
o=bpy.data.objects.new('Estate terrain working',me);bpy.context.collection.objects.link(o)
for poly in me.polygons:poly.use_smooth=True
parts.append(o);export('HM_EstateBackdrop',False)
manifest[-1]['destination']='/Game/Environment/HomeMap/Landscape/Meshes'

# 只替换屋顶原临时长凳，软垫与木条背板使用已有材质。
for x in [-345,-85]:
    for dx in [-92,92]:box((x+dx,-853,760),(12,60,36),'Metal',.8)
    box((x,-850,779),(234,74,5),'Wood',.8)
    for dx in [-58,58]:
        box((x+dx,-846,787),(112,65,12),'Linen',3)
        box((x+dx,-885,819),(112,12,52),'Linen',3)
    for dx in range(-108,109,18):box((x+dx,-896,817),(12,5,64),'Wood',.8)
    for dx in [-119,119]:
        box((x+dx,-853,799),(6,67,8),'Wood',.8)
        box((x+dx,-825,778),(5,5,42),'Metal',.5)
# 茶几降低到 36 cm；脚部留空，从座位看出去没有一整块高石台。
for x in [-288,-142]:
    for y in [-683,-627]:box((x,y,758),(5,5,32),'Metal',.5)
box((-215,-655,775),(176,74,6),'Stone',1.3)
export('HM_RooftopLounge',True)
# 薄木板拼缝帮助读出尺度；与花池合并使用实体碰撞，仅毫米级凸起。
for y in range(-960,0,24):box((-10,y,742.08),(1060,.18,.12),'Metal',.01)
# 两只靠后角的完整花池，主路线 X=260 与中心空地保持畅通。
for x,y in [(-470,-630),(430,-904)]:
    for dx in [-42,42]:box((x+dx,y,770),(6,94,56),'Stone',.6)
    for dy in [-44,44]:box((x,y+dy,770),(78,6,56),'Stone',.6)
    box((x,y,790),(78,82,4),'Metal',.3)
export('HM_RooftopFinish',True)
# 花池具有实体碰撞，不能让角色穿过；地板拼缝只有毫米级高度。
(OUT/'landscape_finish_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
# 远处树带投影到最终网格，UE 用原树网格实例化。
points=[]
estate_bvh=BVHTree.FromObject(bpy.data.objects['HM_EstateBackdrop'],bpy.context.evaluated_depsgraph_get())
for x,y in [(-65,15),(-72,45),(-78,68),(-88,92),(-100,105),(-50,80),(-35,100),(0,115),(22,100),(48,108),(70,88),(85,62),(95,28),(75,-15),(88,-45),(70,-85),(42,-104),(0,-112),(-35,-105),(-75,-72),(-105,-30),(-125,130),(120,145),(155,80)]:
    # 投影到导出的曲面后再摆放，避免解析函数与低密度网格插值高度不同。
    for dx,dy in [(0,0),(6,9),(-7,5)]:
        px,py=x+dx,y+dy
        if max(abs(px),abs(py))<=50:continue
        hit=estate_bvh.ray_cast(Vector((px,-py,2000)),Vector((0,0,-1)))
        assert hit[0] is not None
        points.append([px*100,py*100,hit[0].z*100])
(OUT/'estate_tree_positions.json').write_text(json.dumps(points,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('LANDSCAPE_AND_LOUNGE_SAVED',[(r['mesh'],r['triangles']) for r in manifest])
