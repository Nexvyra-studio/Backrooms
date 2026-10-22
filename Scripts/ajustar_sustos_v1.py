# Ritmo dos sustos: primeiro 1min30 depois do primeiro passo, depois a cada 60-110 s
#   py "C:/Backrooms/Backrooms/Scripts/ajustar_sustos_v1.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DiretorSustos":
        a.set_editor_property("modo_teste", False)
        a.set_editor_property("atraso_apos_primeiro_passo", 90.0)
        a.set_editor_property("intervalo_entre", unreal.Vector2D(60.0, 110.0))
        a.set_editor_property("intervalo_minimo", 45.0)
        unreal.log_warning("[Sustos] %s: primeiro em 90 s, depois a cada 60-110 s (min 45)" % a.get_actor_label())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log_warning("[Sustos] salvo")
