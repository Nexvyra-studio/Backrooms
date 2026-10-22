# Ajustes da revisao (parte 2): audio, visual do Level 0, desempenho e ritmo normal dos sustos
#   py "C:/Backrooms/Backrooms/Scripts/ajustes_v3.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
LINHAS = []


def log(m):
    LINHAS.append(str(m))
    unreal.log_warning("[Ajustes] " + str(m))


# ------------------------------------------------ 1) Audio: os sustos nunca podem ser cortados
prioridades = {
    "Susto_Sombra": 4.0, "Batidas_Parede": 4.0, "Intro_Queda": 4.0,
    "Passo_Carpete_01": 3.0, "Passo_Carpete_02": 3.0, "Passo_Carpete_03": 3.0,
    "Ambiente_Sala": 3.0, "Estalo_Lampada_1": 2.0, "Estalo_Lampada_2": 2.0,
    "Vento_Fresta": 2.0, "Zumbido_Lampada": 0.5,
}
for nome, prio in prioridades.items():
    try:
        w = unreal.load_asset("/Game/Sounds/" + nome)
        if not w:
            log("som nao encontrado: " + nome)
            continue
        w.set_editor_property("priority", prio)
        if nome == "Zumbido_Lampada":
            # So as 12 lampadas mais perto zumbem de verdade (antes eram ~40 brigando pelas 32 vozes)
            c = unreal.SoundConcurrencySettings()
            c.set_editor_property("max_count", 12)
            c.set_editor_property("resolution_rule", unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST)
            w.set_editor_property("override_concurrency", True)
            w.set_editor_property("concurrency_overrides", c)
        EAL.save_loaded_asset(w)
    except Exception as e:
        log("ERRO no som %s: %s" % (nome, e))
log("prioridades de som ajustadas")

# ------------------------------------------------ 2) Visual: mais claro e amarelo (Level 0), luz reage rapido as piscadas
try:
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "PostProcessVolume":
            s = a.get_editor_property("settings")

            def p(nome, valor):
                s.set_editor_property("override_" + nome, True)
                s.set_editor_property(nome, valor)
            p("auto_exposure_bias", 0.0)
            p("color_saturation", unreal.Vector4(0.92, 0.92, 0.92, 1.0))
            p("vignette_intensity", 0.34)
            p("lumen_scene_lighting_update_speed", 2.5)
            p("lumen_final_gather_lighting_update_speed", 2.0)
            a.set_editor_property("settings", s)
            log("pos-processo: exposicao 0, saturacao 0.92, vinheta 0.34, Lumen mais rapido")
except Exception as e:
    log("ERRO no pos-processo: %s" % e)

# ------------------------------------------------ 3) Desempenho: o sol nao entra no Level 0 (tem teto em tudo)
try:
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "DirectionalLight":
            comp = a.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                comp.set_editor_property("cast_shadows", False)
                comp.set_visibility(False)
            a.set_actor_hidden_in_game(True)
            log("luz do sol desligada (%s)" % a.get_actor_label())
except Exception as e:
    log("ERRO na luz do sol: %s" % e)

# ------------------------------------------------ 4) Ritmo normal dos sustos (estava em modo teste: a cada 6-10 s)
for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    if n in ("DiretorSustos", "EnigmaLevel0"):
        try:
            a.set_editor_property("modo_teste", False)
            log("%s: modo teste DESLIGADO" % n)
        except Exception as e:
            log("ERRO %s: %s" % (n, e))

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
log("PRONTO")
try:
    with open("C:/Backrooms/Backrooms/Scripts/resultado_ajustes_v3.txt", "w") as f:
        f.write("\n".join(LINHAS))
except Exception:
    pass
