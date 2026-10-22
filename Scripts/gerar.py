# Gera o Level 0 de novo e salva:  py "C:/Backrooms/Backrooms/Scripts/gerar.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "GeradorLevel0":
        a.call_method("Gerar")
        unreal.log_warning("[Gerar] ok")
        break
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
