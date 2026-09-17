import unreal


PATH = "/Game/UI/Menu/Experiences/W_HostSessionScreen"


def out(message):
    print(f"[SessionUIAudit] {message}")


out(f"parent={unreal.BlueprintService.get_parent_class(PATH)}")
out("widgets:")
for widget in unreal.WidgetService.list_components(PATH):
    out(f"  {widget.widget_name}|{widget.widget_class}|variable={widget.is_variable}|parent={widget.parent_widget}")

for graph in unreal.BlueprintService.list_graphs(PATH):
    out(f"graph={graph.graph_name}|type={graph.graph_kind}|nodes={graph.node_count}")
    for node in unreal.BlueprintService.get_nodes_in_graph(PATH, graph.graph_name):
        out(f"  {node.node_id}|{node.node_type}|{node.node_title}|pins={list(node.pin_names)}")
