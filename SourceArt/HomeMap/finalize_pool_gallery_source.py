"""同步延长花园基座，修复已导出的铭牌镜像文字；保留实体参考板。"""
import bpy,bmesh,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
for r in json.loads((OUT/'architecture.json').read_text(encoding='utf-8')):
    if not r['name'].startswith(('Garden_Lawn_','Foundation_Lawn_')):continue
    if r['name'] in bpy.data.objects:continue
    template='RearGarden_Terrace' if r['name'].startswith('Garden_Lawn_') else 'Foundation_RearGarden'
    o=bpy.data.objects[template].copy();o.name=r['name'];bpy.data.collections['HomeMap_Architecture'].objects.link(o)
    o.location=(r['loc'][0]/100,-r['loc'][1]/100,r['loc'][2]/100);o.scale=r['scale']
    if template=='RearGarden_Terrace':
        o.data=o.data.copy();o.data.materials.clear();o.data.materials.append(bpy.data.objects['Site_Ground'].data.materials[0])
for name,axis in [('HM_ExpeditionSign',1),('HM_FieldNotesBoard',0)]:
    o=bpy.data.objects[name]
    if not o.get('ReadableFaceFixed'):
        for v in o.data.vertices:v.co[axis]*=-1
        bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
    o['ReadableFaceFixed']=True
    o.data.update();bpy.context.view_layer.update()
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    s=bpy.context.scene;s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'
    bpy.ops.object.exportforunreal()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('SOURCE_FINALIZED')
