# Aplica o enigma do Level 0 no projeto (rodar com o Lvl_Level0 aberto):
#   py "C:/Backrooms/Backrooms/Scripts/aplicar_enigma_level0.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log_warning("[Enigma] " + msg)


# 1) Materiais: placa verde de SAIDA (brilha) e papel dos bilhetes
def criar_material(nome, cor, emissivo):
    caminho = "/Game/Materials/" + nome
    if unreal.EditorAssetLibrary.does_asset_exist(caminho):
        log(nome + " ja existe")
        return
    ferramentas = unreal.AssetToolsHelpers.get_asset_tools()
    m = ferramentas.create_asset(nome, "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(cor[0], cor[1], cor[2], 1.0))
    if emissivo:
        m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
        r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
        r.set_editor_property("r", 0.85)
        mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(caminho)
    log(nome + " criado")


try:
    criar_material("M_SaidaVerde", (0.08, 2.2, 0.35), True)
    criar_material("M_Papel", (0.78, 0.76, 0.68), False)
except Exception as e:
    log("ERRO nos materiais: %s" % e)


# 2) Componente de interacao (tecla E) no personagem
try:
    bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
    classe = unreal.load_class(None, "/Script/Backrooms.InteracaoComponent")
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)

    ja_tem = False
    for h in handles:
        dados = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(dados)
        if obj and obj.get_class().get_name() == "InteracaoComponent":
            ja_tem = True
            break

    if ja_tem:
        log("Personagem ja tem o componente de interacao")
    else:
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=classe, blueprint_context=bp)
        novo, motivo = sds.add_new_subobject(params)
        if str(motivo):
            log("ERRO ao adicionar componente: " + str(motivo))
        else:
            sds.rename_subobject(novo, unreal.Text("Interacao"))
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            unreal.EditorAssetLibrary.save_loaded_asset(bp)
            log("Componente Interacao adicionado ao BP_FirstPersonCharacter")
except Exception as e:
    log("ERRO no componente: %s" % e)


# 3) Gera o mapa de novo (com quadros, saida e bilhetes)
gerador = None
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "GeradorLevel0":
        gerador = a
        break

if gerador:
    gerador.call_method("Gerar")
    eas.set_selected_level_actors([gerador])
    log("Mapa gerado de novo")
else:
    log("ERRO: nao achei o GeradorLevel0 no mapa")

contagem = {}
for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    if n in ("QuadroLuz", "ParedeSaida", "Bilhete", "EnigmaLevel0", "DiretorSustos"):
        contagem[n] = contagem.get(n, 0) + 1
log("No mapa: %s" % contagem)

les.save_current_level()
log("Mapa salvo. Pronto.")
