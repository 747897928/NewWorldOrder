"""验证 16 m 泳池新增段的真实碰撞和角色往返；不是游泳系统验收。"""
import unreal,json,time
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn();movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
result={'map':pw.get_path_name(),'water_z':-18,'length_cm':1600,'depth_cm':212,'floor_traces':[],'samples':[],'swimming_implemented':False}
for x,y in [(0,900),(0,1500),(-300,1800),(300,1800),(550,1750),(-550,1750),(0,2000),(1200,1750)]:
    hit=unreal.SystemLibrary.line_trace_single(pw,unreal.Vector(x,y,10),unreal.Vector(x,y,-400),unreal.TraceTypeQuery.ECC_VISIBILITY,False,[pawn],unreal.DrawDebugTrace.NONE).to_dict()
    result['floor_traces'].append({'xy':[x,y],'blocking':hit['blocking_hit'],'z':hit['impact_point'].z,'actor':hit['hit_actor'].get_actor_label() if hit['hit_actor'] else None})
pawn.set_actor_location(unreal.Vector(0,220,90),False,True)
started=time.monotonic();last=0;phase='out'
def extension_tick(dt):
    global last,phase,extension_handle
    now=time.monotonic()-started;pos=pawn.get_actor_location()
    if now-last>.3:
        result['samples'].append({'t':round(now,3),'phase':phase,'position':list(pos.to_tuple()),'mode':str(movement.movement_mode)});last=now
    if phase=='out' and pos.y>1780:result['far_end_reached']=list(pos.to_tuple());phase='return'
    if (phase=='return' and pos.y<220) or now>45:
        movement.stop_movement_immediately();result['end']=list(pos.to_tuple());result['passed']=phase=='return' and pos.y<220 and pos.z>85
        (ROOT/'Docs/Tasks/HomeMap/Reports/pool_extension_walk.json').write_text(json.dumps(result,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(extension_handle);return
    pc.set_control_rotation(unreal.Rotator(pitch=-7,yaw=90 if phase=='out' else -90,roll=0))
    pawn.add_movement_input(unreal.Vector(0,1 if phase=='out' else -1,0),1,True)
extension_handle=unreal.register_slate_post_tick_callback(extension_tick)
print('POOL_EXTENSION_WALK_STARTED')
