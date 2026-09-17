import unreal


DEFAULT_TRANSLATION_TOLERANCE = 1.0e-4
DEFAULT_QUATERNION_DOT_TOLERANCE = 0.999999
DEFAULT_SCALE_TOLERANCE = 1.0e-5


def _load_anim_sequence(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset or asset.get_class().get_name() != "AnimSequence":
        raise RuntimeError("不是有效的 AnimSequence：" + asset_path)
    return asset


def _quaternion_dot(left, right):
    return abs(
        left.x * right.x
        + left.y * right.y
        + left.z * right.z
        + left.w * right.w
    )


def _build_full_local_poses(anim_sequence, key_count):
    options = unreal.AnimPoseEvaluationOptions()
    options.should_retarget = False
    # 临时 Retarget 输出必须是非 Additive 绝对姿势；这里仍显式取 Full Pose，
    # 防止调用者误把 Additive 差值再次写入并在正式资产上二次做差。
    options.retrieve_additive_as_full_pose = True
    return [
        unreal.AnimPoseExtensions.get_anim_pose_at_frame(
            anim_sequence, frame_index, options
        )
        for frame_index in range(key_count)
    ]


def writeback_evaluated_local_tracks(temp_anim_path, formal_anim_path):
    """把临时 Retarget 结果的完整局部骨轨写入既有正式 AnimSequence。

    UE 5.8 的 overwrite_existing_files=True 会 Replace References、Force Delete
    旧资产再重命名新副本。本函数只更新正式资产自己的 AnimationDataController，
    保留 UObject、包路径和外部引用。临时结果必须已经使用目标 Skeleton，并保持
    Additive=None；Additive/Base Pose、曲线、Notify、Sync Marker 等元数据由调用方
    在骨轨验证通过后单独恢复和复读。
    """

    temp_anim = _load_anim_sequence(temp_anim_path)
    formal_anim = _load_anim_sequence(formal_anim_path)

    temp_skeleton = temp_anim.get_editor_property("skeleton")
    formal_skeleton = formal_anim.get_editor_property("skeleton")
    if not temp_skeleton or not formal_skeleton or temp_skeleton != formal_skeleton:
        raise RuntimeError(
            "临时与正式动画 Skeleton 不一致：{} -> {}".format(
                temp_anim_path, formal_anim_path
            )
        )
    if (
        temp_anim.get_editor_property("additive_anim_type")
        != unreal.AdditiveAnimationType.AAT_NONE
    ):
        raise RuntimeError("临时 Retarget 结果必须保持 Additive=None：" + temp_anim_path)

    temp_model = temp_anim.get_editor_property("data_model_interface")
    formal_model = formal_anim.get_editor_property("data_model_interface")
    if not temp_model or not formal_model:
        raise RuntimeError("无法取得 AnimationDataModel 接口")

    temp_track_names = [str(name) for name in temp_model.get_bone_track_names()]
    formal_track_names = [str(name) for name in formal_model.get_bone_track_names()]
    formal_only = [name for name in formal_track_names if name not in temp_track_names]
    if formal_only:
        raise RuntimeError(
            "正式资产存在临时结果没有的骨轨，拒绝静默保留或删除："
            + " | ".join(formal_only)
        )

    key_count = int(temp_model.get_number_of_keys())
    poses = _build_full_local_poses(temp_anim, key_count)
    controller = formal_anim.get_editor_property("controller")
    if not controller:
        raise RuntimeError("正式资产没有 AnimationDataController：" + formal_anim_path)

    # 写入的是目标骨架绝对姿势。先关闭 Additive，避免编辑器在数据变更期间把
    # 新关键帧按旧 Base Pose 解释；成功后由调用方恢复正式 Additive 元数据。
    formal_anim.set_editor_property(
        "additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE
    )
    formal_anim.set_editor_property("ref_pose_seq", None)

    controller.open_bracket("安全写回 Retarget 局部骨轨", should_transact=False)
    try:
        controller.set_frame_rate(
            temp_model.get_frame_rate(), should_transact=False
        )
        controller.set_number_of_frames(
            unreal.FrameNumber(value=temp_model.get_number_of_frames()),
            should_transact=False,
        )

        for track_name in temp_track_names:
            if not formal_model.is_valid_bone_track_name(track_name):
                # UE 5.8 的废弃返回值即使添加成功也可能是 -1，必须复读 Model。
                controller.add_bone_track(track_name, should_transact=False)
                if not formal_model.is_valid_bone_track_name(track_name):
                    raise RuntimeError("添加 Bone Track 失败：" + track_name)

            transforms = [
                unreal.AnimPoseExtensions.get_bone_pose(
                    pose, track_name, unreal.AnimPoseSpaces.LOCAL
                )
                for pose in poses
            ]
            if not controller.set_bone_track_keys(
                track_name,
                [transform.translation for transform in transforms],
                [transform.rotation for transform in transforms],
                [transform.scale3d for transform in transforms],
                should_transact=False,
            ):
                raise RuntimeError("写入 Bone Track 失败：" + track_name)
    finally:
        controller.close_bracket(should_transact=False)

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        formal_anim, only_if_is_dirty=False
    ):
        raise RuntimeError("保存正式 AnimSequence 失败：" + formal_anim_path)

    result = validate_evaluated_local_tracks(temp_anim_path, formal_anim_path)
    print(
        "SAFE_WRITEBACK|{}|{}|TRACKS={}|KEYS={}|MAX_DT={}|MIN_QDOT={}|MAX_DS={}".format(
            temp_anim_path,
            formal_anim_path,
            result["track_count"],
            result["key_count"],
            result["maximum_translation_error"],
            result["minimum_quaternion_dot"],
            result["maximum_scale_error"],
        )
    )
    return result


def validate_evaluated_local_tracks(
    source_anim_path,
    target_anim_path,
    translation_tolerance=DEFAULT_TRANSLATION_TOLERANCE,
    quaternion_dot_tolerance=DEFAULT_QUATERNION_DOT_TOLERANCE,
    scale_tolerance=DEFAULT_SCALE_TOLERANCE,
    excluded_tracks=None,
    excluded_prefixes=None,
):
    source_anim = _load_anim_sequence(source_anim_path)
    target_anim = _load_anim_sequence(target_anim_path)
    source_model = source_anim.get_editor_property("data_model_interface")
    target_model = target_anim.get_editor_property("data_model_interface")

    excluded_tracks = set(excluded_tracks or [])
    excluded_prefixes = tuple(excluded_prefixes or [])
    source_names = [
        str(name)
        for name in source_model.get_bone_track_names()
        if str(name) not in excluded_tracks
        and not str(name).startswith(excluded_prefixes)
    ]
    target_names = [
        str(name)
        for name in target_model.get_bone_track_names()
        if str(name) not in excluded_tracks
        and not str(name).startswith(excluded_prefixes)
    ]
    if source_names != target_names:
        raise RuntimeError("骨轨名称或顺序不一致")
    if source_model.get_number_of_keys() != target_model.get_number_of_keys():
        raise RuntimeError("关键帧数量不一致")
    source_rate = source_model.get_frame_rate()
    target_rate = target_model.get_frame_rate()
    if (
        source_rate.numerator != target_rate.numerator
        or source_rate.denominator != target_rate.denominator
    ):
        raise RuntimeError("帧率不一致")

    key_count = int(source_model.get_number_of_keys())
    source_poses = _build_full_local_poses(source_anim, key_count)
    target_poses = _build_full_local_poses(target_anim, key_count)
    maximum_translation_error = 0.0
    minimum_quaternion_dot = 1.0
    maximum_scale_error = 0.0

    for source_pose, target_pose in zip(source_poses, target_poses):
        for track_name in source_names:
            source_transform = unreal.AnimPoseExtensions.get_bone_pose(
                source_pose, track_name, unreal.AnimPoseSpaces.LOCAL
            )
            target_transform = unreal.AnimPoseExtensions.get_bone_pose(
                target_pose, track_name, unreal.AnimPoseSpaces.LOCAL
            )
            maximum_translation_error = max(
                maximum_translation_error,
                (
                    source_transform.translation - target_transform.translation
                ).length(),
            )
            minimum_quaternion_dot = min(
                minimum_quaternion_dot,
                _quaternion_dot(
                    source_transform.rotation, target_transform.rotation
                ),
            )
            maximum_scale_error = max(
                maximum_scale_error,
                (source_transform.scale3d - target_transform.scale3d).length(),
            )

    if maximum_translation_error > translation_tolerance:
        raise RuntimeError(
            "局部位移误差超限：{}".format(maximum_translation_error)
        )
    if minimum_quaternion_dot < quaternion_dot_tolerance:
        raise RuntimeError(
            "局部旋转误差超限：{}".format(minimum_quaternion_dot)
        )
    if maximum_scale_error > scale_tolerance:
        raise RuntimeError("局部缩放误差超限：{}".format(maximum_scale_error))

    return {
        "track_count": len(source_names),
        "key_count": key_count,
        "maximum_translation_error": maximum_translation_error,
        "minimum_quaternion_dot": minimum_quaternion_dot,
        "maximum_scale_error": maximum_scale_error,
    }


def migrate_mannequin_local_tracks(
    mannequin_source_path, formal_anim_path, track_names
):
    """逐帧迁移指定 Manny 辅助骨的 Local Transform 到正式 CC 动画。

    Retargeter 没有覆盖的 weapon/IK 辅助骨可能产生大位移或末帧尖峰。本函数
    只改调用者明确列出的轨道，保留正式资产其余骨轨、曲线、Notify、Additive
    和 Base Pose 元数据。调用前必须先确认所选辅助骨不需要保留 CC 参考姿势
    偏移；不要把它当作全骨架批量覆盖入口。
    """

    track_names = [str(name) for name in track_names]
    if not track_names or any(not name for name in track_names):
        raise RuntimeError("必须提供至少一个有效辅助骨轨名称")
    if len(track_names) != len(set(track_names)):
        raise RuntimeError("辅助骨轨名称不能重复")

    source_anim = _load_anim_sequence(mannequin_source_path)
    formal_anim = _load_anim_sequence(formal_anim_path)
    source_model = source_anim.get_editor_property("data_model_interface")
    formal_model = formal_anim.get_editor_property("data_model_interface")
    if source_model.get_number_of_keys() != formal_model.get_number_of_keys():
        raise RuntimeError("辅助骨源/目标关键帧数量不一致")
    source_rate = source_model.get_frame_rate()
    formal_rate = formal_model.get_frame_rate()
    if (
        source_rate.numerator != formal_rate.numerator
        or source_rate.denominator != formal_rate.denominator
    ):
        raise RuntimeError("辅助骨源/目标帧率不一致")

    key_count = int(source_model.get_number_of_keys())
    options = unreal.AnimPoseEvaluationOptions()
    options.should_retarget = False
    options.retrieve_additive_as_full_pose = (
        source_anim.get_editor_property("additive_anim_type")
        != unreal.AdditiveAnimationType.AAT_NONE
    )
    source_poses = [
        unreal.AnimPoseExtensions.get_anim_pose_at_frame(
            source_anim, frame_index, options
        )
        for frame_index in range(key_count)
    ]

    controller = formal_anim.get_editor_property("controller")
    controller.open_bracket("迁移 Manny 指定辅助骨轨", should_transact=False)
    try:
        for track_name in track_names:
            transforms = [
                unreal.AnimPoseExtensions.get_bone_pose(
                    pose, track_name, unreal.AnimPoseSpaces.LOCAL
                )
                for pose in source_poses
            ]
            if not formal_model.is_valid_bone_track_name(track_name):
                controller.add_bone_track(track_name, should_transact=False)
                if not formal_model.is_valid_bone_track_name(track_name):
                    raise RuntimeError("添加辅助骨轨失败：" + track_name)
            if not controller.set_bone_track_keys(
                track_name,
                [transform.translation for transform in transforms],
                [transform.rotation for transform in transforms],
                [transform.scale3d for transform in transforms],
                should_transact=False,
            ):
                raise RuntimeError("写入辅助骨轨失败：" + track_name)
    finally:
        controller.close_bracket(should_transact=False)

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        formal_anim, only_if_is_dirty=False
    ):
        raise RuntimeError("保存辅助骨目标动画失败：" + formal_anim_path)

    results = {}
    for frame_index in range(key_count):
        formal_pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(
            formal_anim, frame_index, options
        )
        for track_name in track_names:
            result = results.setdefault(
                track_name,
                {
                    "maximum_translation_error": 0.0,
                    "minimum_quaternion_dot": 1.0,
                    "maximum_scale_error": 0.0,
                },
            )
            source_transform = unreal.AnimPoseExtensions.get_bone_pose(
                source_poses[frame_index],
                track_name,
                unreal.AnimPoseSpaces.LOCAL,
            )
            formal_transform = unreal.AnimPoseExtensions.get_bone_pose(
                formal_pose, track_name, unreal.AnimPoseSpaces.LOCAL
            )
            result["maximum_translation_error"] = max(
                result["maximum_translation_error"],
                (
                    source_transform.translation
                    - formal_transform.translation
                ).length(),
            )
            result["minimum_quaternion_dot"] = min(
                result["minimum_quaternion_dot"],
                _quaternion_dot(
                    source_transform.rotation, formal_transform.rotation
                ),
            )
            result["maximum_scale_error"] = max(
                result["maximum_scale_error"],
                (
                    source_transform.scale3d - formal_transform.scale3d
                ).length(),
            )

    for track_name, result in results.items():
        if (
            result["maximum_translation_error"]
            > DEFAULT_TRANSLATION_TOLERANCE
        ):
            raise RuntimeError(track_name + " 位移误差超限")
        if (
            result["minimum_quaternion_dot"]
            < DEFAULT_QUATERNION_DOT_TOLERANCE
        ):
            raise RuntimeError(track_name + " 旋转误差超限")
        if result["maximum_scale_error"] > DEFAULT_SCALE_TOLERANCE:
            raise RuntimeError(track_name + " 缩放误差超限")
        print(
            "HELPER_TRACK_WRITEBACK|{}|{}|{}|MAX_DT={}|MIN_QDOT={}|MAX_DS={}".format(
                mannequin_source_path,
                formal_anim_path,
                track_name,
                result["maximum_translation_error"],
                result["minimum_quaternion_dot"],
                result["maximum_scale_error"],
            )
        )
    return results


def migrate_weapon_r_local_track(mannequin_source_path, formal_anim_path):
    """兼容旧调用点：只迁移 Manny 的 weapon_r Local Transform。"""

    return migrate_mannequin_local_tracks(
        mannequin_source_path, formal_anim_path, ["weapon_r"]
    )["weapon_r"]
