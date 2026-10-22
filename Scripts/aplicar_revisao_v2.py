# Aplica a revisao completa do Level 0 (rodar com o Lvl_Level0 aberto, depois de compilar):
#   py "C:/Backrooms/Backrooms/Scripts/aplicar_revisao_v2.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary


LINHAS = []


def log(m):
    LINHAS.append(m)
    unreal.log_warning("[Revisao] " + m)


# ---------------------------------------------------------------- 1) Sons novos
sons = ["Ambiente_Sala", "Vento_Fresta", "Batidas_Parede", "Intro_Queda"]
tarefas = []
for nome in sons:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "C:/Backrooms/Backrooms/Audio/%s.wav" % nome)
    t.set_editor_property("destination_path", "/Game/Sounds")
    t.set_editor_property("destination_name", nome)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    tarefas.append(t)
try:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tarefas)
    for nome in ("Ambiente_Sala", "Vento_Fresta"):
        w = unreal.load_asset("/Game/Sounds/" + nome)
        if w:
            w.set_editor_property("looping", True)
            EAL.save_loaded_asset(w)
    log("sons importados: %s" % ", ".join(n for n in sons if EAL.does_asset_exist("/Game/Sounds/" + n)))
except Exception as e:
    log("ERRO ao importar sons: %s" % e)


# ---------------------------------------------------------------- 2) Desempenho do zumbido
try:
    z = unreal.load_asset("/Game/Sounds/Zumbido_Lampada")
    if z:
        # Fora do alcance o som vira "virtual" (custo quase zero) e volta certinho ao chegar perto
        z.set_editor_property("virtualization_mode", unreal.VirtualizationMode.RESTART)
        z.set_editor_property("looping", True)
        EAL.save_loaded_asset(z)
    att = unreal.load_asset("/Game/Sounds/ATT_Lampada")
    if att:
        cfg = att.get_editor_property("attenuation")
        log("ATT_Lampada: raio interno %.0f, queda %.0f" % (cfg.get_editor_property("attenuation_shape_extents").x, cfg.get_editor_property("falloff_distance")))
        if cfg.get_editor_property("falloff_distance") > 1600.0:
            cfg.set_editor_property("falloff_distance", 1400.0)
            att.set_editor_property("attenuation", cfg)
            EAL.save_loaded_asset(att)
            log("ATT_Lampada: queda ajustada para 1400")
except Exception as e:
    log("ERRO no zumbido: %s" % e)


# ---------------------------------------------------------------- 3) Balanco da camera no personagem
try:
    bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
    classe = unreal.load_class(None, "/Script/Backrooms.BalancoCameraComponent")
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)
    ja_tem = False
    for h in handles:
        dados = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(dados)
        if obj and obj.get_class().get_name() == "BalancoCameraComponent":
            ja_tem = True
    if classe and not ja_tem:
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=classe, blueprint_context=bp)
        novo, motivo = sds.add_new_subobject(params)
        if str(motivo):
            log("ERRO ao adicionar balanco: " + str(motivo))
        else:
            sds.rename_subobject(novo, unreal.Text("BalancoCamera"))
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            EAL.save_loaded_asset(bp)
            log("BalancoCamera adicionado ao personagem")
    else:
        log("BalancoCamera: %s" % ("ja existia" if ja_tem else "classe NAO encontrada (compilou?)"))
except Exception as e:
    log("ERRO no balanco: %s" % e)


# ---------------------------------------------------------------- 4) Pos-processamento (look de camera real)
try:
    volumes = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "PostProcessVolume"]
    if not volumes:
        v = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        v.set_actor_label("PosProcesso_Level0")
        volumes = [v]
        log("PostProcessVolume criado")
    for a in volumes:
        if True:
            s = a.get_editor_property("settings")
            def p(nome, valor):
                s.set_editor_property("override_" + nome, True)
                s.set_editor_property(nome, valor)
            p("film_grain_intensity", 0.22)          # grao fino de camera
            p("vignette_intensity", 0.42)            # cantos mais escuros
            p("scene_fringe_intensity", 0.18)        # aberracao cromatica leve (lente barata)
            p("bloom_intensity", 0.55)               # paineis de luz "estouram" um pouco
            p("motion_blur_amount", 0.15)
            p("lens_flare_intensity", 0.0)
            a.set_editor_property("settings", s)
            a.set_editor_property("unbound", True)
            log("pos-processamento ajustado em " + a.get_actor_label())
except Exception as e:
    log("ERRO no pos-processamento: %s" % e)


# ---------------------------------------------------------------- 5) Gera o mapa de novo
try:
    achou = False
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "GeradorLevel0":
            a.call_method("Gerar")
            log("mapa gerado")
            achou = True
            break
    if not achou:
        log("ERRO: GeradorLevel0 nao encontrado no mapa aberto")

    contagem = {}
    for a in eas.get_all_level_actors():
        n = a.get_class().get_name()
        if n in ("QuadroLuz", "ParedeSaida", "Bilhete", "EnigmaLevel0", "DiretorSustos", "AmbienteLevel0", "LampadaPainel", "PostProcessVolume"):
            contagem[n] = contagem.get(n, 0) + 1
    log("no mapa: %s" % contagem)

    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "DiretorSustos":
            for prop in ("intervalo_entre", "intervalo_minimo", "atraso_apos_primeiro_passo", "volume_passos", "peso_batidas"):
                try:
                    log("DiretorSustos.%s = %s" % (prop, a.get_editor_property(prop)))
                except Exception as e2:
                    log("DiretorSustos.%s ? (%s)" % (prop, e2))

    mundo = unreal.EditorLevelLibrary.get_editor_world()
    log("mapa aberto: %s" % mundo.get_path_name())
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log("PRONTO")
except Exception as e:
    log("ERRO na geracao: %s" % e)

try:
    with open("C:/Backrooms/Backrooms/Scripts/resultado_revisao.txt", "w") as f:
        f.write("\n".join(LINHAS))
except Exception:
    pass
