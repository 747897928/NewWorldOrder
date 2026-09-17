"""中远景林带：复用既有叶片图集，用覆盖率控制几何预算，不使用实心球冠。"""
import bpy,bmesh,json,math,random
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
checkpoint=OUT.parents[1]/'Saved/HomeMapCheckpoints/HomeMap_BeforeWoodland.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
collection=bpy.data.collections.get('Estate_Woodland_Source')
if collection is None:
 collection=bpy.data.collections.new('Estate_Woodland_Source');bpy.context.scene.collection.children.link(collection)
source=bpy.data.objects['HM_CourtyardCanopyTree'];manifest=[]
for variant in range(2):
 rng=random.Random(481+variant);name='HM_EstateWoodland_'+str(variant)
 old=bpy.data.objects.get(name)
 if old:bpy.data.objects.remove(old,do_unlink=True)
 verts=[];faces=[];indices=[];uvs=[]
 def branch(a,b,r0,r1):
  a,b=Vector(a),Vector(b);axis=(b-a).normalized();u=axis.cross(Vector((0,1,0))).normalized();v=axis.cross(u).normalized();start=len(verts)
  for pos,r in [(a,r0),(b,r1)]:
   for i in range(6):verts.append(pos+r*(math.cos(i*math.tau/6)*u+math.sin(i*math.tau/6)*v))
  for i in range(6):
   j=(i+1)%6;faces.append((start+i,start+j,start+6+j,start+6+i));indices.append(0);uvs.append(((i/6,0),(j/6,0),(j/6,(b-a).length),(i/6,(b-a).length)))
 def leaf(center,length,width):
  normal=Vector((rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-.3,1))).normalized();u=normal.cross(Vector((0,1,0))).normalized();v=normal.cross(u).normalized()
  outline=[(0,-.5),(-.4,-.22),(-.5,.15),(0,.5),(.5,.15),(.4,-.22)]
  uv=[(.264,.615),(.189,.689),(.165,.835),(.276,.973),(.327,.835),(.311,.689)]
  start=len(verts);verts.append(center+normal*length*.05)
  for x,y in outline:verts.append(center+u*x*width+v*y*length)
  for i in range(6):faces.append((start,start+1+i,start+1+(i+1)%6));indices.append(1);uvs.append(((.247,.799),uv[i],uv[(i+1)%6]))
 # 粗干和分枝保证从屋顶看见的树冠有支撑；省去远景不可见的小枝。
 branch((0,0,0),(.1,0,4.5),.3,.19);branch((.1,0,4.5),(0,.1,8.2),.19,.045)
 for arm in range(10):
  angle=arm*2.39996+variant*.7;radius=2.5 if arm<8 else 1.0
  centre=Vector((math.cos(angle)*radius,math.sin(angle)*radius,6.5+(arm%3)*.9))
  branch((.1,0,3.4+(arm%3)*.8),centre,.11,.025)
  for i in range(85):
   p=Vector((rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-1,1)))
   while p.length>1:p=Vector((rng.uniform(-1,1),rng.uniform(-1,1),rng.uniform(-1,1)))
   centre_leaf=centre+Vector((p.x*2.0,p.y*1.9,p.z*1.7))
   leaf(centre_leaf,rng.uniform(.55,.9),rng.uniform(.30,.48))
 me=bpy.data.meshes.new('SM_'+name);me.from_pydata(verts,[],faces);me.update()
 for m in source.data.materials:me.materials.append(m)
 layer=me.uv_layers.new(name='UVMap')
 for polygon,index,coords in zip(me.polygons,indices,uvs):
  polygon.material_index=index;polygon.use_smooth=index==0
  for li,uv in zip(polygon.loop_indices,coords):layer.data[li].uv=uv
 o=bpy.data.objects.new(name,me);collection.objects.link(o)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh'/('SM_'+name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
 me.calc_loop_triangles()
 manifest.append({'mesh':'SM_'+name,'triangles':len(me.loop_triangles),'materials':['/Game/Environment/HomeMap/Materials/Hub/M_EstateWoodlandBark','/Game/Environment/HomeMap/Materials/Hub/M_EstateWoodlandLeaf'],'nanite':False,'collision':False,'rebuild_material_slots':True,'destination':'/Game/Environment/HomeMap/Landscape/Meshes','instances':[]})
 o.hide_set(True);o.hide_render=True
# 固定种子和地形射线保持实例位置可复现；不会进入中心 100×100 m 场地。
tree=BVHTree.FromObject(bpy.data.objects['HM_EstateBackdrop'],bpy.context.evaluated_depsgraph_get())
rng=random.Random(718);points=[[],[]];accepted=[]
centres=[(-95,55,26),(-140,125,32),(-50,140,30),(40,170,38),(130,120,33),(110,20,28),(150,-85,33),(30,-140,35),(-90,-100,30),(-190,-25,38)]
for cx,cy,radius in centres:
 count=0
 for attempt in range(500):
  angle=rng.uniform(0,math.tau);r=radius*math.sqrt(rng.random());x=cx+math.cos(angle)*r;y=cy+math.sin(angle)*r
  if max(abs(x),abs(y))<65 or any((x-a)**2+(y-b)**2<5.8**2 for a,b in accepted):continue
  hit=tree.ray_cast(Vector((x,-y,1000)),Vector((0,0,-1)))
  if hit[0] is None:continue
  accepted.append((x,y));points[count%2].append([round(x*100,2),round(y*100,2),round(hit[0].z*100-25,2)])
  count+=1
  if count>=34:break
(OUT/'estate_woodland_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
(OUT/'estate_woodland_positions.json').write_text(json.dumps(points,indent=2)+'\n',encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('WOODLAND_SAVED',[(r['mesh'],r['triangles']) for r in manifest],'instances',list(map(len,points)))