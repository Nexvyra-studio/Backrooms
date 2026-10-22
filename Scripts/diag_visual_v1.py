# Diagnostico: exposicao, pos-processo, sons e modos de teste
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = []
def log(m):
    L.append(str(m))
for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    if n == "PostProcessVolume":
        s = a.get_editor_property("settings")
        for p in ("auto_exposure_method", "auto_exposure_bias", "auto_exposure_min_brightness", "auto_exposure_max_brightness",
                  "auto_exposure_apply_physical_camera_exposure", "auto_exposure_speed_up", "auto_exposure_speed_down",
                  "lumen_scene_lighting_update_speed", "lumen_final_gather_lighting_update_speed", "lumen_final_gather_quality",
                  "dynamic_global_illumination_method", "reflection_method", "white_temp", "color_saturation", "color_gamma",
                  "ambient_occlusion_intensity", "vignette_intensity", "film_grain_intensity", "bloom_intensity", "indirect_lighting_intensity"):
            try:
                ov = ""
                try:
                    ov = " (override=%s)" % s.get_editor_property("override_" + p)
                except Exception:
                    pass
                log("PP %s = %s%s" % (p, s.get_editor_property(p), ov))
            except Exception as e:
                log("PP %s ? %s" % (p, e))
    elif n in ("DiretorSustos", "EnigmaLevel0"):
        log("%s modo_teste = %s" % (n, a.get_editor_property("modo_teste")))
    elif n in ("SkyLight", "DirectionalLight", "ExponentialHeightFog", "SkyAtmosphere", "PlayerStart"):
        log("ator %s em %s" % (n, a.get_actor_location()))
for nome in ("Zumbido_Lampada", "Susto_Sombra", "Batidas_Parede", "Ambiente_Sala", "Intro_Queda", "Vento_Fresta"):
    w = unreal.load_asset("/Game/Sounds/" + nome)
    if w:
        try:
            log("som %s: prioridade %.2f, duracao %.2f, classe %s" % (nome, w.get_editor_property("priority"), w.get_editor_property("duration"), w.get_editor_property("sound_class_object")))
        except Exception as e:
            log("som %s ? %s" % (nome, e))
try:
    cfg = unreal.WindowsTargetSettings.get_default_object()
    log("AudioMaxChannels = %s" % cfg.get_editor_property("audio_max_channels"))
except Exception as e:
    log("AudioMaxChannels ? %s" % e)
for p in ("/Game/Sounds", "/Game/Materials", "/Game/Maps", "/Game/Meshes"):
    log("%s: %s" % (p, ", ".join(x.split(".")[-1] for x in unreal.EditorAssetLibrary.list_assets(p, recursive=False))))
with open("C:/Backrooms/Backrooms/Scripts/resultado_diag.txt", "w") as f:
    f.write("\n".join(L))
unreal.log_warning("[Diag] pronto")
