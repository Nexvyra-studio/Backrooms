# Primeiro susto = apagao; passos mais altos e mais perto
#   py "C:/Backrooms/Backrooms/Scripts/ajustar_sustos_v2.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DiretorSustos":
        a.set_editor_property("primeiro_susto", unreal.TipoSusto.APAGAO)
        a.set_editor_property("volume_passos", 2.2)
        a.set_editor_property("distancia_passos", unreal.Vector2D(450.0, 750.0))
        unreal.log_warning("[Sustos] %s: primeiro = apagao, passos volume 2.2 a 4,5-7,5 m" % a.get_actor_label())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log_warning("[Sustos] salvo")
