"""低预算背景阔叶树：按树冠覆盖率分配叶片，避免全局减面把叶片消成碎点。"""
import bpy,math,random,json,os
from mathutils import Vector
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeCanopyAndJoinery.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
old=bpy.data.objects.get('HM_CourtyardCanopyTree')
if old:bpy.data.objects.remove(old,do_unlink=True)
random.seed(41)
verts=[];faces=[];indices=[];uvfaces=[]
def branch(a,b,r0,r1,sides=7):
    a,b=Vector(a),Vector(b);axis=(b-a).normalized();u=axis.cross(Vector((0,1,0))).normalized();v=axis.cross(u).normalized()
    start=len(verts)
    for pos,r in [(a,r0),(b,r1)]:
        for i in range(sides):verts.append(pos+r*(math.cos(i*math.tau/sides)*u+math.sin(i*math.tau/sides)*v))
    for i in range(sides):
        j=(i+1)%sides;faces.append((start+i,start+j,start+sides+j,start+sides+i));indices.append(0);uvfaces.append(((i/sides,0),(j/sides,0),(j/sides,(b-a).length),(i/sides,(b-a).length)))
def leaf(center,length,width):
    normal=Vector((random.uniform(-.7,.7),random.uniform(-.7,.7),random.uniform(.25,1))).normalized()
    u=normal.cross(Vector((0,1,0))).normalized();v=normal.cross(u).normalized()
    theta=random.random()*math.tau;u,v=math.cos(theta)*u+math.sin(theta)*v,-math.sin(theta)*u+math.cos(theta)*v
    # 轮廓对准已有叶片图集中的第二片叶；几何裁掉背景，不引入大面积透明卡片。
    outline=[(0,-.5),(-.40,-.22),(-.5,.15),(0,.5),(.5,.15),(.40,-.22)]
    uv=[(.264,.615),(.189,.689),(.165,.835),(.276,.973),(.327,.835),(.311,.689)]
    base=len(verts);center=Vector(center)
    verts.append(center+normal*length*.055)
    for x,y in outline:verts.append(center+u*x*width+v*y*length)
    for i in range(6):faces.append((base,base+1+i,base+1+(i+1)%6));indices.append(1);uvfaces.append(((.247,.799),uv[i],uv[(i+1)%6]))
stem=[(0,0,0),(.10,.04,1.8),(-.08,.09,3.3),(.18,0,4.7),(.1,.1,6.1)]
for i in range(4):branch(stem[i],stem[i+1],.19-i*.037,.153-i*.035,9)
for arm in range(12):
    theta=arm*math.tau/12+random.uniform(-.15,.15)
    height=3.0+(arm%4)*.44
    a=Vector((.02,0,height));b=Vector((math.cos(theta)*1.65,math.sin(theta)*1.65,height+.95))
    branch(a,b,.07,.026)
    for twig in range(4):
        phi=theta+(twig-1.5)*.32
        end=b+Vector((math.cos(phi)*random.uniform(.45,1.1),math.sin(phi)*random.uniform(.45,1.1),random.uniform(.25,1.15)))
        branch(b,end,.025,.006,6)
        # 叶片沿末梢实际生长，避免大叶片随机悬浮在枝条外；每簇独立分叉。
        for shoot in range(8):
            root=b.lerp(end,.35+.6*shoot/8)
            az=phi+random.uniform(-1.9,1.9)
            tip=end+Vector((math.cos(az)*random.uniform(.25,.78),math.sin(az)*random.uniform(.25,.78),random.uniform(-.18,.62)))
            branch(root,tip,.008,.0015,5)
            axis=(tip-root).normalized();side=axis.cross(Vector((0,0,1))).normalized()
            for i in range(12):
                attach=root.lerp(tip,.24+.74*i/12)
                sign=-1 if i%2 else 1
                center=attach+side*sign*random.uniform(.045,.11)
                leaf(center,random.uniform(.14,.23),random.uniform(.07,.13))
mesh=bpy.data.meshes.new('SM_HM_CourtyardCanopyTree');mesh.from_pydata(verts,[],faces);mesh.update()
tree=bpy.data.objects.new('HM_CourtyardCanopyTree',mesh);bpy.context.collection.objects.link(tree)
source=bpy.data.objects['SM_HM_IslandTree']
for token in ['island_tree_01','island_tree_01_leaves']:
    m=next(m for m in source.data.materials if m.name==token or m.name.startswith(token+'.'))
    mesh.materials.append(m)
uvlayer=mesh.uv_layers.new(name='UVMap')
for p,idx,uv in zip(mesh.polygons,indices,uvfaces):
    p.material_index=idx;p.use_smooth=idx==0
    for li,co in zip(p.loop_indices,uv):uvlayer.data[li].uv=co
tree.location=(-22,0,-1.48)
tree['purpose']='外围背景树冠；使用原项目树叶图集，按覆盖率建模；非全局 Decimate'
# 只导出本树，局部坐标保持树根为原点；展示位置不进入 FBX。
bpy.ops.object.select_all(action='DESELECT');tree.select_set(True);bpy.context.view_layer.objects.active=tree
saved_location=tree.location.copy();tree.location=(0,0,0)
bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh/SM_HM_CourtyardCanopyTree.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
tree.location=saved_location;mesh.calc_loop_triangles()
report={'triangles':len(mesh.loop_triangles),'leaf_count':4608,'materials':[m.name for m in mesh.materials],'new_textures':0,'method':'Connected fine shoots with smaller leaves; existing 2k leaf atlas; built-in FBX local root pivot'}
(OUT/'canopy_tree_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print(json.dumps(report))
