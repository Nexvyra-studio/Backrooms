import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "SombraCorredor":
        eas.destroy_actor(a)
        n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log_warning("[Sombra] %d previa(s) removida(s), mapa salvo" % n)
