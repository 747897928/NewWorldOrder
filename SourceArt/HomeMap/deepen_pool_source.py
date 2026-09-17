"""只加深现有泳池，保留庭院标高；当前 Master 必须已保存为可回退提交。"""
import bpy, json, os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceArt/HomeMap'
assert Path(bpy.data.filepath).resolve()==(OUT/'HomeMap_Master.blend').resolve()
records=json.loads((OUT/'architecture.json').read_text(encoding='utf-8'))
by_name={r['name']:r for r in records}

def place(name,loc,scale,template=None):
    if name not in by_name:
        r=dict(by_name[template]);r['name']=name;records.append(r);by_name[name]=r
    r=by_name[name];r.update(loc=loc,scale=scale)
    o=bpy.data.objects.get(name)
    if o is None:
        o=bpy.data.objects[template].copy();o.name=name
        bpy.data.collections['HomeMap_Architecture'].objects.link(o)
    o.location=(loc[0]/100,-loc[1]/100,loc[2]/100);o.scale=scale

# 池底顶面 -230 cm，水面仍为 -18 cm，主游泳区水深 212 cm。
place('Pool_Base',[0,790,-245],[1,1,1])
for x in [-440,440]:place('Pool_Side_'+str(x),[x,790,-115],[1,1,230/120])
for y in [280,1300]:place('Pool_End_'+str(y),[0,y,-115],[1,1,230/120])
# 沿用原入口，20 cm 踢面、30 cm 踏面，最后到池底落差 10 cm。
for i in range(11):
    name='Pool_Step_'+str(i);template='Pool_Step_'+str(min(i,6))
    original_height=by_name.get(name,by_name[template])['dims'][2]
    top=-20*(i+1);height=230+top
    place(name,[0,300+i*30,(top-230)/2],[1,1,height/original_height],template)

# 原外围地形在 -150 cm，会穿过加深后的池壳。仅下沉被建筑基座遮住的池下网格。
o=bpy.data.objects['Site_Ground']
if o.data.name!='SM_HM_SiteTerrain_DeepPool':
    o.data=o.data.copy();o.data.name='SM_HM_SiteTerrain_DeepPool'
count=0
for v in o.data.vertices:
    if abs(v.co.x)<=6.001 and -14.001<=v.co.y<=-1.999:
        v.co.z=-2.7;count+=1
o.data.update()
by_name['Site_Ground'].update(mesh='SM_HM_SiteTerrain_DeepPool',asset_path='/Game/Environment/HomeMap/Architecture/Hub/SM_HM_SiteTerrain_DeepPool')
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
o.name='HM_SiteTerrain_DeepPool';o.bfu_export_type='export_self_only';o.bfu_rotate_to_zero_for_export=True;o.bfu_auto_generate_collision=False
s=bpy.context.scene;s.bfu_unreal_import_location='Environment/HomeMap/Architecture/Hub';s.bfu_export_static_mesh_file_path=str(OUT/'BFEU_Production/StaticMesh')+os.sep
bpy.ops.object.exportforunreal();o.name='Site_Ground'
(OUT/'architecture.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('POOL_DEEPENED',{'depth_cm':212,'steps':11,'lowered_terrain_vertices':count})
