import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
old=bpy.data.objects.get('Clerestory_Side_550')
if old:bpy.data.objects.remove(old,do_unlink=True)
p=OUT/'architecture.json';r=json.loads(p.read_text(encoding='utf-8'));r=[x for x in r if x['name']!='Clerestory_Side_550'];p.write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('REPLACED_EAST_CLERESTORY_REMOVED')
