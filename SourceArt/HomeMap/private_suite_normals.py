"""对本轮套件重算法线并仅重导这三个网格。"""
import bpy,bmesh,os
from pathlib import Path
OUT=Path(__file__).resolve().parent
for name in ['HM_BedroomHeadwall','HM_BedroomCurtain','HM_BathVanitySuite']:
    o=bpy.data.objects[name];bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.object.exportforunreal()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
