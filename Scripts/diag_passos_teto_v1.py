import unreal
L = []
MEL = unreal.MaterialEditingLibrary
# 1) Teto: parametros do material
for caminho in ("/Game/Materials/MI_Teto", "/Game/Materials/MI_Parede", "/Game/Materials/MI_Carpete"):
    mi = unreal.load_asset(caminho)
    if not mi:
        continue
    L.append("%s (pai %s)" % (caminho, mi.get_editor_property("parent").get_path_name() if mi.get_editor_property("parent") else "?"))
    for n in MEL.get_scalar_parameter_names(mi):
        L.append("  escalar %s = %s" % (n, MEL.get_material_instance_scalar_parameter_value(mi, n)))
    for n in MEL.get_vector_parameter_names(mi):
        L.append("  vetor %s = %s" % (n, MEL.get_material_instance_vector_parameter_value(mi, n)))
    for n in MEL.get_texture_parameter_names(mi):
        t = MEL.get_material_instance_texture_parameter_value(mi, n)
        L.append("  textura %s = %s" % (n, t.get_path_name() if t else None))
# 2) Personagem: componentes
bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for h in sds.k2_gather_subobject_data_for_blueprint(bp):
    d = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    o = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(d)
    if not o:
        continue
    L.append("componente %s (%s)" % (o.get_name(), o.get_class().get_name()))
    if o.get_class().get_name() == "PassosComponent":
        for p in ("sons_passo", "distancia_passo", "volume", "velocidade_minima", "fade_ao_parar"):
            try:
                v = o.get_editor_property(p)
                if p == "sons_passo":
                    v = [s.get_name() if s else None for s in v]
                L.append("   %s = %s" % (p, v))
            except Exception as e:
                L.append("   %s ? %s" % (p, e))
# animacoes com notify de som?
for caminho in unreal.EditorAssetLibrary.list_assets("/Game/FirstPerson", recursive=True):
    if "Sound" in caminho or "Foot" in caminho or "Step" in caminho:
        L.append("asset FirstPerson: " + caminho)
# 3) Sons de passo
for nome in ("Passo_Carpete_01", "Passo_Carpete_02", "Passo_Carpete_03"):
    w = unreal.load_asset("/Game/Sounds/" + nome)
    if w:
        L.append("%s: duracao %.2f s, looping %s" % (nome, w.get_editor_property("duration"), w.get_editor_property("looping")))
with open("C:/Backrooms/Backrooms/Scripts/resultado_diag2.txt", "w") as f:
    f.write("\n".join(L))
unreal.log_warning("[Diag2] pronto")
