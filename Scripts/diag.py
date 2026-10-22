import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "Bilhete":
        t = str(a.get_editor_property("texto"))
        esc = a.get_component_by_class(unreal.TextRenderComponent)
        unreal.log_warning("[Diag] %s | tamanho=%s | %r" % (a.get_actor_label(), esc.get_editor_property("world_size"), t))
