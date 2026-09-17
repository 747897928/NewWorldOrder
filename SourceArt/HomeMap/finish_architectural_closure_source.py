"""与正式关卡同步被替代的建筑实例；共享网格资产不删除。"""
import bpy,json
from pathlib import Path
OUT=Path(__file__).resolve().parent
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
removed=['Living_Glass_350','Living_Sliding_Glass_1','Living_Sliding_Glass_2','Living_Sliding_Glass_3','HM_LivingSlidingDoorDetail','Clerestory_South','Clerestory_Side_-550']
for n in removed:
 o=bpy.data.objects.get(n)
 if o:bpy.data.objects.remove(o,do_unlink=True)
for n in ['Living_Mullion_140','Living_Mullion_570']:
 o=bpy.data.objects[n];o.location.z=5.21;o.dimensions.z=3.38
path=OUT/'architecture.json';rows=json.loads(path.read_text(encoding='utf-8'))
rows=[r for r in rows if r['name'] not in removed]
for r in rows:
 if r['name'] in ['Living_Mullion_140','Living_Mullion_570']:
  r['loc'][2]=521;r['dims'][2]=338
path.write_text(json.dumps(rows,indent=2),encoding='utf-8')
for n,x in [('HM_GalleryCurtain_West',-5.15),('HM_GalleryCurtain_East',5.35)]:
 o=bpy.data.objects.get(n)
 if o:o.location.x=x
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('CLOSURE_SOURCE_SYNCHRONIZED')
