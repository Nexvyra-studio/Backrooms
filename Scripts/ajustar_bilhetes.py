# Ajusta os bilhetes ja gerados: tira linhas em branco e aumenta a letra
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "Bilhete":
        t = str(a.get_editor_property("texto"))
        while "\n\n" in t:
            t = t.replace("\n\n", "\n")
        a.set_editor_property("texto", unreal.Text(t))
        esc = a.get_component_by_class(unreal.TextRenderComponent)
        esc.set_editor_property("world_size", 1.8)
        esc.set_text(unreal.Text(t))
        n += 1
unreal.log_warning("[Diag] %d bilhetes ajustados" % n)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
