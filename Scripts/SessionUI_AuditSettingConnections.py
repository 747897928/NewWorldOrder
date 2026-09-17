import unreal


TARGETS = [
    ("/Game/UI/Settings/Editors/W_SettingsListEntry_Action", "EventGraph"),
    ("/Game/UI/Settings/Editors/W_SettingsListEntry_SubCollection", "EventGraph"),
    ("/Game/UI/Settings/W_LyraSettingScreen", "RegisterTopLevelTab"),
    ("/Game/UI/FrontEnd/W_FrontEnd", "EventGraph"),
]


def log(message):
    unreal.log(f"[SessionUIConnectionAudit] {message}")
    print(f"[SessionUIConnectionAudit] {message}")


for path, graph in TARGETS:
    log(f"GRAPH {path}:{graph}")
    for node in unreal.BlueprintService.get_nodes_in_graph(path, graph):
        log(f"NODE {node.node_id}|{node.node_type}|{node.node_title}|{list(node.pin_names)}")
        for pin in unreal.BlueprintService.get_node_pins(path, graph, node.node_id):
            if pin.default_value:
                log(f"DEFAULT {node.node_id}.{pin.pin_name}={pin.default_value}")
    for connection in unreal.BlueprintService.get_connections(path, graph):
        log(f"CONN {connection.source_node_id}.{connection.source_pin_name} -> "
            f"{connection.target_node_id}.{connection.target_pin_name}")
