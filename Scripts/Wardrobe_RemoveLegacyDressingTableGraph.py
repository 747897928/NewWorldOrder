import unreal


BLUEPRINT_PATH = "/Game/Assets/Furniture/Dressing_Table_Set/Blueprint/BP_Dressing_Table_Set"
GRAPH_NAME = "EventGraph"

# 这些 GUID 来自 2026-09-01 迁移前的图表审计。删除前逐项核对标题，避免误删用户后来重建的节点。
LEGACY_NODES = {
    "70FAFD3E43CE8F03A22C4089E8AE1E9E": "Event BeginPlay",
    "41B1AF49406D704E5C43CC8E6F939778": "Event ActorBeginOverlap",
    "5777728A4A69D3F381372BB3556190D0": "Event Tick",
    "7B6D1F6B4837F491D0CC61AF0D71A9F5": "Event Interact",
    "8B42FC8D4B8CB0E8533FB7A83FC42ED7": "Get Controller",
    "1F542B884CDCADA0EB7D86B4F3036E24": "PushContentToLayerForPlayer",
    "BDB3EF9246EBD9820B54D397FB5B19C2": "Cast To BP_ShootPlayerController",
    "E3A532504DD09DFBD5B0E6A0145E7C1E": "Send Gameplay Event to Actor",
    "9D7A02EC4824E5822021EDB61E1FF7E2": "Remove Gameplay Tag",
    "10E2692840C9BBB9D9D11283AAAA77CC": "Reroute Node",
    "42E5BAB44BEF1C87A6BCFCB79DAB5486": "Reroute Node",
    "91D8820141CD3A3E336C66A4B6E1222A": "Reroute Node",
    "48C9EC5B43EADB8E32644B883F418408": "Reroute Node",
}


def normalize_title(title):
    return str(title).split("\n", 1)[0].strip()


blueprint = unreal.load_asset(BLUEPRINT_PATH)
if blueprint is None:
    raise RuntimeError(f"Cannot load Blueprint: {BLUEPRINT_PATH}")

nodes_by_id = {
    str(node.node_id): node
    for node in unreal.BlueprintService.get_nodes_in_graph(BLUEPRINT_PATH, GRAPH_NAME)
}
deleted = []
for node_id, expected_title in LEGACY_NODES.items():
    node = nodes_by_id.get(node_id)
    if node is None:
        continue
    actual_title = normalize_title(node.node_title)
    if actual_title != expected_title:
        raise RuntimeError(
            f"Refusing to delete changed node {node_id}: "
            f"expected={expected_title} actual={actual_title}")
    if not unreal.BlueprintService.delete_node(BLUEPRINT_PATH, GRAPH_NAME, node_id):
        raise RuntimeError(f"Cannot delete legacy node: {node_id} {actual_title}")
    deleted.append(f"{node_id}:{actual_title}")

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if blueprint.get_editor_property("status") != unreal.BlueprintStatus.BS_UP_TO_DATE:
    raise RuntimeError(f"Blueprint compile failed: {blueprint.get_editor_property('status')}")
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
    raise RuntimeError("Cannot save BP_Dressing_Table_Set")

remaining_titles = [
    normalize_title(node.node_title)
    for node in unreal.BlueprintService.get_nodes_in_graph(BLUEPRINT_PATH, GRAPH_NAME)
]
for forbidden in (
        "Event Interact", "PushContentToLayerForPlayer", "Cast To BP_ShootPlayerController",
        "Send Gameplay Event to Actor", "Remove Gameplay Tag"):
    if forbidden in remaining_titles:
        raise RuntimeError(f"Legacy node remains after save: {forbidden}")

print(f"[WardrobeDressingTable] Deleted legacy nodes={len(deleted)}")
for item in deleted:
    print(f"[WardrobeDressingTable] {item}")
print(f"[WardrobeDressingTable] Remaining EventGraph nodes={len(remaining_titles)}")
