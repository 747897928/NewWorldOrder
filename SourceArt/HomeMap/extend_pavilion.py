import bpy, json, math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
master=OUT/'HomeMap_Master.blend'
assert Path(bpy.data.filepath).resolve()==master.resolve(), '必须在已保存主文件上增量制作'
bpy.ops.wm.save_as_mainfile(filepath=str(master))
checkpoint=OUT/'HomeMap_BeforeGallery.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
records=json.loads((OUT/'architecture.json').read_text(encoding='utf-8'))
by_name={r['name']:r for r in records}
collection=bpy.data.collections['HomeMap_Architecture']
new_meshes={}

def box(name,loc,dims,mat='Oak',folder='Pavilion',collision=True):
    mesh_name='SM_HM_'+'_'.join(str(v).replace('.','p') for v in dims)
    mesh=bpy.data.meshes.get(mesh_name)
    if not mesh:
        bpy.ops.mesh.primitive_cube_add(size=1)
        temp=bpy.context.object
        temp.dimensions=[v/100 for v in dims]
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        bevel=temp.modifiers.new('Edge highlights','BEVEL');bevel.width=min(.008,min(dims)/600);bevel.segments=3
        bpy.ops.object.modifier_apply(modifier=bevel.name)
        mesh=temp.data;mesh.name=mesh_name
        uv=mesh.uv_layers.active
        for p in mesh.polygons:
            axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
            for li in p.loop_indices:
                v=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(v[axes[0]],v[axes[1]])
        bpy.data.objects.remove(temp,do_unlink=True)
        new_meshes[mesh_name]=mesh
    ob=bpy.data.objects.get(name)
    if ob:ob.data=mesh
    else:ob=bpy.data.objects.new(name,mesh);collection.objects.link(ob)
    ob.location=(loc[0]/100,-loc[1]/100,loc[2]/100)
    if not mesh.materials:mesh.materials.append(bpy.data.materials.get('Preview_'+mat) or bpy.data.materials['Preview_Plaster'])
    ob['unreal_material']=mat;ob['unreal_folder']=folder
    r=dict(name=name,mesh=mesh_name,loc=loc,dims=dims,mat=mat,folder=folder,collision=collision)
    if name in by_name:by_name[name].update(r)
    else:records.append(r);by_name[name]=r
    return ob

# 保持一层 U 型平面，仅增高中央展厅；左右翼屋顶完全不动。
box('Roof_Living',(0,-500,710),(1200,1080,40),'Plaster','Roof')
box('Clerestory_South',(0,-1000,510),(1100,6,360),'Glass','Glazing')
for x in [-550,550]:box('Clerestory_Side_'+str(x),(x,-500,510),(6,1000,360),'Glass','Glazing')
for x in [-350,350]:box('Living_Glass_'+str(x),(x,0,162.5),(420,4,325),'Glass','Glazing')
for x in [-570,-140,140,570]:box('Living_Mullion_'+str(x),(x,0,345),(7,10,690),'Metal','Glazing')
box('Living_Facade_Beam',(0,0,340),(1160,22,28),'Metal','Pavilion')
box('Living_Canopy',(0,90,340),(1160,180,24),'Stone','Gallery')
box('Gallery_Rear_Floor',(0,-810,340),(1100,380,24),'Oak','Gallery')
box('Gallery_West_Walk',(-450,-310,340),(200,620,24),'Oak','Gallery')
box('Pavilion_High_Canopy',(0,95,705),(1200,190,20),'Oak','Roof')
for x in [-585,585]:
    box('Pavilion_Frame_'+str(x),(x,5,350),(40,65,700),'Stone','Pavilion')
    box('Pavilion_Reveal_'+str(x),(x-12*(1 if x>0 else -1),-31,350),(3,3,680),'Glow','Lighting',False)
box('Pavilion_Crown',(0,5,697),(1210,70,45),'Stone','Pavilion')
box('Pavilion_Crown_Reveal',(0,-33,674),(1160,3,3),'Glow','Lighting',False)
box('Upper_Living_Glass',(110,0,525),(860,4,330),'Glass','Glazing')

# 110 cm 高玻璃护栏；侧廊净宽约 190 cm，后方 Gallery 深 380 cm。
for name,loc,dims in [
 ('Gallery_Rear_Guard',(-10,-612,407),(660,3,110)),
 ('Gallery_West_Guard',(-348,-310,407),(3,600,110)),
 ('Balcony_Front_Guard',(0,178,407),(1130,3,110)),
 ('Balcony_West_Guard',(-570,90,407),(3,180,110)),
 ('Balcony_East_Guard',(570,90,407),(3,180,110))]:
    box(name,loc,dims,'Glass','Gallery')
    cap=list(loc);cap[2]=465;cd=list(dims);cd[2]=4
    box(name+'_Cap',cap,cd,'Metal','Gallery')
for x in [-320,0,320]:box('Gallery_Guard_Post_'+str(x),(x,-612,407),(4,4,110),'Metal','Gallery')

# 180 cm 宽直跑楼梯，20 级，每级 17.6 cm。UCX 斜坡让 CharacterMovement 平顺上行。
stairs_name='SM_HM_GalleryStair'
mesh=bpy.data.meshes.get(stairs_name)
if not mesh:
    pieces=[]
    for i in range(20):
        bpy.ops.mesh.primitive_cube_add(size=1)
        ob=bpy.context.object;ob.dimensions=(1.8,.30,(i+1)*.176)
        ob.location=(0,(-285+i*30)/100,((i+1)*17.6/2-176)/100)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);pieces.append(ob)
    bpy.ops.object.select_all(action='DESELECT')
    for ob in pieces:ob.select_set(True)
    bpy.context.view_layer.objects.active=pieces[0];bpy.ops.object.join()
    ob=bpy.context.object;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    mesh=ob.data;mesh.name=stairs_name
    bevel=ob.modifiers.new('Tread nosing highlights','BEVEL');bevel.width=.004;bevel.segments=2
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    bpy.data.objects.remove(ob,do_unlink=True)
    new_meshes[stairs_name]=mesh
ob=bpy.data.objects.get('Gallery_Stair')
if not ob:ob=bpy.data.objects.new('Gallery_Stair',mesh);collection.objects.link(ob)
ob.location=(4.3,3.0,1.76)
if not mesh.materials:mesh.materials.append(bpy.data.materials['Preview_Oak'])
r=dict(name='Gallery_Stair',mesh=stairs_name,loc=[430,-300,176],dims=[180,600,352],mat='Oak',folder='Gallery',collision=True)
if r['name'] in by_name:by_name[r['name']].update(r)
else:records.append(r)
for x in [330,530]:
    for i in range(0,21,2):box('Stair_Post_'+str(x)+'_'+str(i),(x,-i*30,70+i*17.6),(4,4,110),'Metal','Gallery')
    rail=box('Stair_Rail_'+str(x),(x,-300,296),(5,695.85,5),'Metal','Gallery')
    rail.rotation_euler.x=math.radians(30.386)
    by_name['Stair_Rail_'+str(x)]['rotation']=[0,0,-30.386]

# 玄关到室外场地的十级宽台阶，替代 150 cm 高差的直接跌落。
for i in range(10):box('Arrival_Step_'+str(i),(0,-1570-i*35,-7.5-i*15),(900,35,15),'Stone','Courtyard')

# 北端小型观景廊与背景花池，增加层次但不堵住泳池环形通路。
for x in [-500,500]:box('Garden_Pergola_Post_'+str(x),(x,1640,145),(14,14,290),'Metal','Landscape')
for y in [1510,1720]:box('Garden_Pergola_Beam_'+str(y),(0,y,290),(1040,12,18),'Metal','Landscape')
for x in range(-500,501,40):box('Garden_Pergola_Slat_'+str(x),(x,1615,301),(12,240,12),'Oak','Landscape')
box('Garden_Bench',(0,1640,28),(800,80,56),'Oak','Landscape')

(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(master))
bpy.ops.object.select_all(action='DESELECT')
for name,mesh in new_meshes.items():
    temp=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(temp)
    temp.select_set(True);bpy.context.view_layer.objects.active=temp
    collision=None
    if name==stairs_name:
        # 本地坐标用 Blender Y 正向上升；碰撞为封闭三角棱柱。
        vs=[(-.9,-3,-1.76),(.9,-3,-1.76),(-.9,3,-1.76),(.9,3,-1.76),(-.9,3,1.76),(.9,3,1.76)]
        fs=[(0,2,3,1),(2,4,5,3),(0,1,5,4),(0,4,2),(1,3,5)]
        cm=bpy.data.meshes.new('UCX_GalleryStair');cm.from_pydata(vs,[],fs);cm.update()
        collision=bpy.data.objects.new('UCX_'+name+'_00',cm);bpy.context.scene.collection.objects.link(collision);collision.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/f'{name}.fbx'),use_selection=True,
        object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
    bpy.data.objects.remove(temp,do_unlink=True)
    if collision:bpy.data.objects.remove(collision,do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=str(master))
print('SAVED_GALLERY',len(records),'EXPORTED',len(new_meshes))
