# Passos novos (1 passo por arquivo), teto alinhado com as lampadas, lampada apagada clara
#   py "C:/Backrooms/Backrooms/Scripts/ajustes_v4.py"
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = []
def log(m):
    L.append(str(m)); unreal.log_warning("[Ajustes4] " + str(m))

# 1) Importa os 6 passos (substitui os 3 antigos, que tinham varios passos cada e volumes muito diferentes)
tarefas = []
for i in range(1, 7):
    nome = "Passo_Carpete_%02d" % i
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "C:/Backrooms/Backrooms/Audio/%s.wav" % nome)
    t.set_editor_property("destination_path", "/Game/Sounds")
    t.set_editor_property("destination_name", nome)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    tarefas.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tarefas)
sons = []
for i in range(1, 7):
    w = unreal.load_asset("/Game/Sounds/Passo_Carpete_%02d" % i)
    if w:
        w.set_editor_property("priority", 3.0)
        EAL.save_loaded_asset(w)
        sons.append(w)
log("passos importados: %d (duracoes %s)" % (len(sons), ", ".join("%.2f" % s.get_editor_property("duration") for s in sons)))

# 2) Personagem: usa os 6 passos, volume mais baixo (os arquivos novos sao bem mais fortes)
bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for h in sds.k2_gather_subobject_data_for_blueprint(bp):
    o = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    if o and o.get_class().get_name() == "PassosComponent":
        o.set_editor_property("sons_passo", sons)
        o.set_editor_property("volume", 0.42)
        log("personagem: 6 passos, volume 0.42")
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_loaded_asset(bp)

# 3) Passos do susto: mesmos 6 sons; volume ajustado (o arquivo antigo mais usado era 30x mais baixo)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DiretorSustos":
        a.set_editor_property("sons_passos", sons)
        a.set_editor_property("volume_passos", 1.1)
        log("sustos: 6 passos, volume 1.1")

# 4) Teto: placas de 60 cm (a sala tem 360 cm = 6 placas) -> lampadas e paredes caem certinho nas linhas
mi = unreal.load_asset("/Game/Materials/MI_Teto")
MEL.set_material_instance_scalar_parameter_value(mi, "TamanhoTextura", 360.0)
EAL.save_loaded_asset(mi)
log("teto: placas de 60 cm")

# 5) Lampada apagada/queimada: difusor branco (antes ficava um quadrado escuro no teto)
m = unreal.load_asset("/Game/Materials/M_LuminariaApagada")
n = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
if n and n.get_class().get_name() == "MaterialExpressionConstant3Vector":
    n.set_editor_property("constant", unreal.LinearColor(0.78, 0.77, 0.72, 1.0))
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    log("lampada apagada: difusor claro")

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
log("PRONTO")
with open("C:/Backrooms/Backrooms/Scripts/resultado_ajustes_v4.txt", "w") as f:
    f.write("\n".join(L))
