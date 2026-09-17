"""浅水入口保持 -230 cm；池底坡降到 -518 cm，深水 5 m。只执行环境增量。"""
import bpy,bmesh,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
records=json.loads((OUT/'architecture.json').read_text(encoding='utf-8'));rows={r['name']:r for r in records}
def place(n,loc,scale,template=None):
    if n not in rows:
        rows[n]=dict(rows[template]);rows[n]['name']=n;records.append(rows[n])
    o=bpy.data.objects.get(n)
    if o is None:
        o=bpy.data.objects[template].copy();o.name=n;bpy.data.collections['HomeMap_Architecture'].objects.link(o)
    o.location=(loc[0]/100,-loc[1]/100,loc[2]/100);o.scale=scale;rows[n].update(loc=loc,scale=scale)
    return o
for n in ['Pool_Side_-440','Pool_Side_440','Pool_End_280','Pool_End_1300']:
    r=rows[n];loc=list(r['loc']);loc[2]=-259;scale=list(r.get('scale',[1,1,1]));scale[2]=518/120;place(n,loc,scale)
def export(o,name):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    old=o.name;o.name=name.removeprefix('SM_');o.data.update();bpy.context.view_layer.update()
    bpy.ops.export_scene.fbx(filepath=str(OUT/'BFEU_Production/StaticMesh'/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Z',axis_up='Y',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
    o.name=old;o.data.calc_loop_triangles();return len(o.data.loop_triangles)
# 单一连续池底，没有通过透明水面掩盖断面或悬空台阶。
v=[]
for y,z in [(280,-230),(640,-230),(940,-518),(1900,-518)]:
    for x in [-440,440]:v.append((x/100,-y/100,z/100))
for y in [280,640,940,1900]:
    for x in [-440,440]:v.append((x/100,-y/100,-5.48))
f=[]
for i in range(3):
    j=i*2;f.extend([(j,j+1,j+3,j+2),(j+8,j+10,j+11,j+9),(j,j+2,j+10,j+8),(j+1,j+9,j+11,j+3)])
f.extend([(0,8,9,1),(6,7,15,14)])
o=bpy.data.objects['Pool_Base'];oldm=list(o.data.materials)
me=bpy.data.meshes.new('SM_HM_PoolDeepShell');me.from_pydata(v,[],f);me.update();o.data=me;o.location=(0,0,0);o.scale=(1,1,1);o.rotation_euler=(0,0,0)
for m in oldm:me.materials.append(m)
bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
uv=me.uv_layers.new(name='UVMap')
for face in me.polygons:
    axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(face.normal[j]))]
    for li in face.loop_indices:
        co=me.vertices[me.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
pooltris=export(o,'SM_HM_PoolDeepShell')
rows['Pool_Base'].update(mesh='SM_HM_PoolDeepShell',asset_path='/Game/Environment/HomeMap/Architecture/Hub/SM_HM_PoolDeepShell',loc=[0,0,0],scale=[1,1,1])
terrain=bpy.data.objects['Site_Ground'];count=0
for vert in terrain.data.vertices:
    if abs(vert.co.x)<=6.001 and -20.001<=vert.co.y<=-1.999:vert.co.z=-5.9;count+=1
terraintris=export(terrain,'SM_HM_SiteTerrain_DeepPool')
manifest=[]
for name,actor,mat,tris in [('SM_HM_PoolDeepShell','HM_Pool_Base','M_Pool_Mosaic',pooltris),('SM_HM_SiteTerrain_DeepPool','HM_Site_Ground','M_HomeGardenSurface',terraintris)]:
    manifest.append({'mesh':name,'triangles':tris,'materials':['/Game/Environment/HomeMap/Materials/Hub/'+mat],'nanite':False,'collision':True,'instances':[{'label':actor,'location_cm':[0,0,0],'rotation':[0,0,0],'scale':[1,1,1]}]})
(OUT/'diving_pool_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('DEEP_POOL_SOURCE_SAVED',{'deep_cm':500,'pool_triangles':pooltris,'lowered_terrain_vertices':count})
