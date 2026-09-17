"""复用窗帘并修正新吊灯的平滑面；不重建既有家具。"""
import bpy,json,os
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
curtains=[('HM_GalleryCurtain_West',[-435,-979,353]),('HM_GalleryCurtain_East',[460,-979,353]),('HM_LivingCurtain_Front',[-506,-20,0])]
for name,loc in curtains:
    o=bpy.data.objects.get(name)
    if not o:
        o=bpy.data.objects['HM_BedroomCurtain'].copy();o.name=name;bpy.context.collection.objects.link(o)
    o.location=(loc[0]/100,-loc[1]/100,loc[2]/100);o.rotation_euler[2]=1.5707963267948966;o.scale=(1,1,1.05 if loc[2]>0 else 1.05)
o=bpy.data.objects['HM_PavilionChandelier']
for f in o.data.polygons:
    # 竖向侧壁平滑，水平灯环顶底保持平面法线。
    if len(f.vertices)==4:f.use_smooth=abs(f.normal.z)<.1
o.data.update();bpy.context.view_layer.update();bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.object.exportforunreal()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('PAVILION_SOURCE_SAVED',curtains)
print('RELATED_SOURCE',[o.name for o in bpy.data.objects if any(w in o.name.lower() for w in ['living_table','living_cup','window_vase','living_books'])])
