# Monta o Level 0 no mapa aberto (Lvl_Level0).
# Rodar no Unreal: Cmd -> Python ->  py "C:/Backrooms/Backrooms/Scripts/montar_level0.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def log(msg):
    unreal.log_warning("[Level0] " + msg)

# 1) Remove o que sobrou do mapa antigo (gerador antigo, lampadas e paredes de teste)
nomes_teste = {"Floor", "Teto", "Parede_1", "Parede_2", "Parede_3", "Parede_4"}
classes_apagar = {"GeradorLabirinto", "GeradorLevel0", "Lampada", "LampadaPainel", "DiretorSustos"}

apagar = []
for a in eas.get_all_level_actors():
    cls = a.get_class().get_name()
    rotulo = a.get_actor_label()
    if (cls in classes_apagar or cls.startswith("BP_Lampada") or rotulo in nomes_teste
            or a.actor_has_tag("GeradoPeloLabirinto") or a.actor_has_tag("GeradoPeloLevel0")):
        apagar.append(a)

for a in apagar:
    eas.destroy_actor(a)
log("Removidos %d atores antigos" % len(apagar))

# 2) Garante um PlayerStart
tem_start = any(a.get_class().get_name() == "PlayerStart" for a in eas.get_all_level_actors())
if not tem_start:
    eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
    log("PlayerStart criado")

# 3) Forro com placas de 60 cm (alinha com paredes e lampadas)
mi_teto = unreal.load_asset("/Game/Materials/MI_Teto")
if mi_teto:
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi_teto, "TamanhoTextura", 300.0)
    unreal.EditorAssetLibrary.save_loaded_asset(mi_teto)
    log("MI_Teto: TamanhoTextura = 300")

# 4) Coloca o gerador do Level 0 na origem e gera
classe = unreal.load_class(None, "/Script/Backrooms.GeradorLevel0")
if not classe:
    log("ERRO: classe GeradorLevel0 nao encontrada (o projeto foi compilado?)")
else:
    gerador = eas.spawn_actor_from_class(classe, unreal.Vector(0, 0, 0))
    gerador.set_actor_label("GeradorLevel0")
    gerador.call_method("Gerar")
    log("Gerado!")

    # Seleciona o gerador para aparecer no Details
    eas.set_selected_level_actors([gerador])

# 5) Salva o mapa
les.save_current_level()
log("Mapa salvo. Pronto.")
