"""从入口沿原楼梯、侧门、外楼梯到屋顶，再从高跳台入水；仅起点归位。"""
import unreal,time,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
pw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(pw,unreal.PlayerController) if p.get_controlled_pawn())
pawn=pc.get_controlled_pawn()
assert 'ShootCharacter' in pawn.get_class().get_name(), '必须使用项目实际角色'
movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
movement.stop_movement_immediately()
# 仅起点归位；后续全程通过 CharacterMovement，不瞬移越过碰撞。
pawn.set_actor_location(unreal.Vector(0,-1430,90),False,False)
targets=[[0,-800],[0,-320],[180,100],[360,170],[360,-745],[420,-820],[665,-820],[670,-690],[670,75],[440,75],[440,-240],[-230,-500],[260,-100],[260,940]]
result={'map':pw.get_path_name(),'targets':targets,'samples':[],'reached':0,'jump_requested':False,'saw_falling':False,'saw_swimming':False}
started=time.monotonic();last=0
def terrace_tick(dt):
    global last,terrace_handle
    now=time.monotonic()-started;pos=pawn.get_actor_location();mode=str(movement.movement_mode)
    if result['jump_requested']:
        result['saw_falling']|='FALLING' in mode
        result['saw_swimming']|='SWIMMING' in mode
    if now-last>.08:
        anim=pawn.mesh.get_anim_instance();montage=anim.get_current_active_montage() if anim else None
        result['samples'].append({'t':round(now,3),'position':list(pos.to_tuple()),'target':result['reached'],'mode':mode,'montage':montage.get_name() if montage else None});last=now
    if result['saw_swimming'] or now>150:
        movement.stop_movement_immediately();pawn.stop_jumping()
        result['passed']=result['reached']==len(targets) and result['saw_falling'] and result['saw_swimming']
        result['end']=list(pos.to_tuple());result['final_mode']=mode
        (ROOT/'Docs/Tasks/HomeMap/Reports/rooftop_route_walk.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(terrace_handle);return
    if result['reached']==len(targets):
        if not result['jump_requested']:
            result['jump_requested']=True;pawn.jump()
        pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=90,roll=0));pawn.add_movement_input(unreal.Vector(0,1,0),1,True);return
    x,y=targets[result['reached']];dx,dy=x-pos.x,y-pos.y;distance=math.hypot(dx,dy)
    if distance<18:result['reached']+=1;return
    # 后台节流曾使回调间隔达到 0.33 s，满输入每帧跨过目标并来回振荡。靠近目标降低测试输入，不改角色速度。
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=math.degrees(math.atan2(dy,dx)),roll=0));pawn.add_movement_input(unreal.Vector(dx/distance,dy/distance,0),min(1,distance/150),True)
terrace_handle=unreal.register_slate_post_tick_callback(terrace_tick)
print('ROOFTOP_ROUTE_TEST_STARTED')
