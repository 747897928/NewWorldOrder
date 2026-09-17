import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
retired=['Wardrobe_Cabinet_730','Wardrobe_Front_730','Wardrobe_Cabinet_890','Wardrobe_Front_890','Mission_Cabinet_-1060','Mission_Cabinet_-840']
for name in retired:
 o=bpy.data.objects.get(name)
 if o:bpy.data.objects.remove(o,do_unlink=True)
p=OUT/'architecture.json';rows=json.loads(p.read_text(encoding='utf-8'));p.write_text(json.dumps([r for r in rows if r['name'] not in retired],indent=2),encoding='utf-8')
r=json.loads((OUT/'canopy_tree_report.json').read_text(encoding='utf-8'));r.update({'nanite':False,'actual_ue_triangles_lod0':32304,'fallback_note':'Rebuilt via StaticMeshEditorSubsystem.set_nanite_settings(apply_changes=True); ordinary mesh avoids inflated sparse foliage.'})
(OUT/'canopy_tree_report.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
(OUT/'BFEU_Production/StaticMesh/SM_HM_CourtyardCanopyTree_additional_data.json').write_text(json.dumps({'exporter':'Blender built-in FBX','axis_forward':'-Z','axis_up':'Y','units':'meters','pivot':'root','triangles':32304,'materials':r['materials']},indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('JOINERY_SOURCE_SAVED')
