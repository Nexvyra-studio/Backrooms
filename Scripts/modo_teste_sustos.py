# Liga/desliga o modo teste dos sustos (sem salvar o mapa):
#   py "C:/Backrooms/Backrooms/Scripts/modo_teste_sustos.py" on
#   py "C:/Backrooms/Backrooms/Scripts/modo_teste_sustos.py" off
import sys
import unreal
ligar = len(sys.argv) > 1 and sys.argv[1] == "on"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DiretorSustos":
        a.set_editor_property("modo_teste", ligar)
        unreal.log_warning("[Sustos] modo teste %s" % ("LIGADO" if ligar else "DESLIGADO"))
if not ligar:
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log_warning("[Sustos] mapa salvo")
