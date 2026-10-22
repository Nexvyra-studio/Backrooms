# Gera o Level 0 de novo e deixa pronto para testar o enigma:
#   py "C:/Backrooms/Backrooms/Scripts/preparar_teste.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def log(m):
    unreal.log_warning("[Teste] " + m)

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "GeradorLevel0":
        a.call_method("Gerar")
        log("mapa gerado")
        break

for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    if n == "EnigmaLevel0":
        a.set_editor_property("modo_teste", True)
        try:
            log("Enigma em modo teste; rota com %d celulas" % len(a.get_editor_property("caminho_saida")))
        except Exception as e:
            log("Enigma em modo teste (rota: %s)" % e)
    elif n == "DiretorSustos":
        a.set_editor_property("modo_teste", False)
        log("Sustos em modo normal")

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
log("salvo")
