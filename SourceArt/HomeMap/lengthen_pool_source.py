"""将现有 10 m 泳池向花园延长为 16 m；不移动 U 型住宅。仅运行一次。"""
import bpy, json, os
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve() == (OUT / 'HomeMap_Master.blend').resolve()
records = json.loads((OUT / 'architecture.json').read_text(encoding='utf-8'))
by_name = {r['name']: r for r in records}
assert by_name['Pool_Base']['loc'][1] == 790, '已延长或基准已改变，禁止重复偏移'
changed = []
for r in records:
    n = r['name']; delta = 0; length = None
    if n == 'Pool_Base' or n.startswith('Pool_Side_'): delta, length = 300, 1620
    elif n in ['Pool_Coping_-451','Pool_Coping_451']: delta, length = 300, 1640
    elif n in ['Deck_Side_-570','Deck_Side_570','Foundation_DeckWest','Foundation_DeckEast']: delta, length = 300, 1750
    elif n.startswith('Garden_Wall_'): delta, length = 300, 3600
    elif n in ['Pool_End_1300','Pool_Coping_1310','Deck_North','Foundation_DeckNorth','Foundation_RearGarden','RearGarden_Terrace','Garden_North_Wall','Garden_Bench'] or n.startswith(('Planter_North_', 'Planter_Soil_', 'Garden_Pergola_')): delta = 600
    if not delta: continue
    r['loc'][1] += delta
    scale = list(r.get('scale',[1,1,1]))
    if length is not None: scale[1] = length / r['dims'][1]
    r['scale'] = scale
    o = bpy.data.objects[n]
    o.location = (r['loc'][0]/100,-r['loc'][1]/100,r['loc'][2]/100)
    o.scale = scale
    changed.append(n)

# 只下沉延长池壳下方的地形；房屋和外围地貌保持现状。
o = bpy.data.objects['Site_Ground']
for v in o.data.vertices:
    if abs(v.co.x) <= 6.001 and -20.001 <= v.co.y <= -1.999: v.co.z = -2.7
o.data.update()
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
o.name='HM_SiteTerrain_DeepPool'; o.bfu_export_type='export_self_only'; o.bfu_rotate_to_zero_for_export=True; o.bfu_auto_generate_collision=False
s=bpy.context.scene; s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub'; s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep
bpy.ops.object.exportforunreal(); o.name='Site_Ground'
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'pool_extension_records.json').write_text(json.dumps(changed,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('POOL_LENGTHENED',len(changed),'architectural instances; 16 m water length')
