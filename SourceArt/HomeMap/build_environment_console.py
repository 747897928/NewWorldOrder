"""在已验证 Blender 源文件上增量制作住宅控制台，BFEU 仅导出此新模型。"""
import bpy,math,json,os
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve().is_relative_to(OUT)
checkpoint=OUT/'HomeMap_BeforeConsole_20260906.blend'
if not checkpoint.exists():bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint),copy=True)
records=json.loads((OUT/'architecture.json').read_text(encoding='utf-8'))
collection=bpy.data.collections['HomeMap_Architecture']
for r in records:
    ob=bpy.data.objects.get(r['name'])
    if ob is None and r['name'].startswith('Living_Sliding_Glass_'):
        ob=bpy.data.objects.new(r['name'],bpy.data.objects['Living_Glass_350'].data);collection.objects.link(ob)
    if ob is None:continue
    if any(t in r['name'] for t in ['Stair','Rear_Guard','Guard_Post','Living_Glass_350','Living_Sliding_Glass_']):
        ob.location=(r['loc'][0]/100,-r['loc'][1]/100,r['loc'][2]/100)
        ob.scale=r.get('scale',[1,1,1])
        # BFEU Copy Transform 已验证 X 欧拉旋转对应 UE 正 Roll。
        if r['name'].startswith('Stair_Rail'):ob.rotation_euler=(math.radians(r['rotation'][2]),0,0)

palette=[('Console_Wood',(.16,.065,.025,1),.45,0),('Console_Obsidian',(.012,.016,.02,1),.3,.65),('Console_Brass',(.42,.23,.075,1),.3,.85),('Console_Engraving',(.8,.54,.2,1),.45,0)]
mats=[]
for name,color,rough,metal in palette:
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color=color;m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=color;p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    mats.append(m)
parts=[]
def finish(ob,name,mat,bevel=0):
    ob.name=name
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    ob.data.materials.clear();ob.data.materials.append(mats[mat])
    if bevel:
        mod=ob.modifiers.new('Edge highlight','BEVEL');mod.width=bevel;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(ob);return ob
def box(name,loc,dims,mat,bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    ob=bpy.context.object;ob.dimensions=dims
    return finish(ob,name,mat,bevel)
box('Console plinth',(0,0,.025),(.44,.32,.05),1,.008)
box('Console brass reveal',(0,0,.063),(.32,.22,.016),2,.002)
box('Console walnut body',(0,0,.565),(.34,.24,.99),0,.014)
for i in range(9):box('Walnut flute '+str(i),(-.14+i*.035,.126,.57),(.022,.021,.88),0,.005)
box('Rear dark inset',(0,-.124,.57),(.27,.008,.80),1,.003)
tilt=Matrix.Rotation(math.radians(-20),4,'X');center=Vector((0,0,1.1))
def panelpoint(p):return center+tilt.to_3x3()@Vector(p)
ob=box('Inclined control head',panelpoint((0,0,0)),(.49,.36,.04),2,.014);ob.rotation_euler[0]=math.radians(-20)
ob=box('Inset black panel',panelpoint((0,0,.026)),(.46,.33,.018),1,.009);ob.rotation_euler[0]=math.radians(-20)
def disc(name,x,y,z,radius,depth,mat):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=radius,depth=depth,location=panelpoint((x,y,z)))
    ob=finish(bpy.context.object,name,mat,.0015);ob.rotation_euler[0]=math.radians(-20)
    for p in ob.data.polygons:
        if len(p.vertices)==4:p.use_smooth=True
    return ob
disc('Dial brass surround',0,.037,.044,.083,.012,2)
disc('Rotary dial',0,.037,.056,.07,.025,1)
disc('Dial brass center',0,.037,.070,.045,.004,2)
ob=box('Dial pointer',panelpoint((0,.082,.074)),(.005,.028,.003),3,.001);ob.rotation_euler[0]=math.radians(-20)
def stroke(name,points,radius=.0015,mat=3):
    curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.resolution_u=1;curve.bevel_depth=radius;curve.bevel_resolution=1
    spline=curve.splines.new('POLY');spline.points.add(len(points)-1)
    for pt,p in zip(spline.points,points):pt.co=(*panelpoint(p),1)
    ob=bpy.data.objects.new(name,curve);bpy.context.collection.objects.link(ob)
    bpy.context.view_layer.objects.active=ob;ob.select_set(True)
    bpy.ops.object.convert(target='MESH');finish(bpy.context.object,name,mat)
def ring(name,x,y,r):stroke(name,[(x+r*math.cos(i*math.tau/32),y+r*math.sin(i*math.tau/32),.040) for i in range(33)])
ring('Day sun',-.145,-.083,.014)
for i in range(8):
    t=i*math.tau/8;stroke('Sun ray',[(-.145+r*math.cos(t),-.083+r*math.sin(t),.040) for r in [.021,.028]])
stroke('Dusk horizon',[(-.03,-.087,.040),(.03,-.087,.040)])
stroke('Dusk sun',[(.019*math.cos(i*math.pi/20),-.087-.019*math.sin(i*math.pi/20),.040) for i in range(21)])
stroke('Night crescent',[(.145+.022*math.cos(t),-.083+.022*math.sin(t),.040) for t in [math.radians(50+i*260/28) for i in range(29)]])
for x,word in [(-.145,'DAY'),(0,'DUSK'),(.145,'NIGHT')]:
    bpy.ops.object.text_add(location=panelpoint((x,-.045,.041)))
    ob=bpy.context.object;ob.data.body=word;ob.data.align_x='CENTER';ob.data.size=.015;ob.data.extrude=0;ob.data.resolution_u=2
    # 从玩家所在的正 Y 侧阅读，文字在面板内旋转 180 度。
    ob.rotation_euler=(math.radians(-20),0,math.pi)
    bpy.ops.object.convert(target='MESH');finish(bpy.context.object,'Legend '+word,3)
bpy.ops.object.select_all(action='DESELECT')
for ob in parts:ob.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join()
console=bpy.context.object;console.name='HM_EnvironmentConsole';console.data.name='SM_HM_EnvironmentConsole'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
console.location=(1.70,6.90,0)
console['purpose']='住宅昼夜控制台；实例使用已有 IA_Interact，不增加输入映射'
console['unreal_location_cm']=[170,-690,0]
# UV 使用米制局部投影，复用 UE 木材；不新增大尺寸贴图。
uv=console.data.uv_layers.active or console.data.uv_layers.new(name='UVMap')
for p in console.data.polygons:
    axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
    for li in p.loop_indices:
        co=console.data.vertices[console.data.loops[li].vertex_index].co;uv.data[li].uv=(co[axes[0]],co[axes[1]])
console.data.calc_loop_triangles()
report={'triangles':len(console.data.loop_triangles),'material_slots':[m.name for m in console.data.materials],'blender_location':list(console.location),'ue_location':[170,-690,0],'export':'BFEU Rotate To Zero, local geometry'}
scene=bpy.context.scene
exportdir=OUT/'BFEU_Production/StaticMesh';exportdir.mkdir(parents=True,exist_ok=True)
scene.bfu_export_static_mesh_file_path=str(exportdir)+os.sep
scene.bfu_unreal_import_module='Game';scene.bfu_unreal_import_location='Environment/HomeMap/Props/Hub'
scene.bfu_export_selection_filter='only_object'
for key in ['static_collection','skeletal','animation','alembic','groom_simulation','camera','spline']:setattr(scene,'bfu_use_'+key+'_export',False)
scene.bfu_use_static_export=True;scene.bfu_use_text_import_asset_script=True;scene.bfu_use_text_additional_data=True
console.bfu_export_type='export_self_only';console.bfu_rotate_to_zero_for_export=True
console.bfu_auto_generate_collision=True
bpy.ops.object.select_all(action='DESELECT');console.select_set(True);bpy.context.view_layer.objects.active=console
print('EXPORT',bpy.ops.object.exportforunreal())
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
(OUT/'console_report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
