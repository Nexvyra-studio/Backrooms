import unreal
MEL = unreal.MaterialEditingLibrary
L = []
for caminho in ("/Game/Materials/M_LuminariaApagada", "/Game/Materials/M_Luminaria"):
    m = unreal.load_asset(caminho)
    if not m:
        L.append("%s: nao existe" % caminho)
        continue
    L.append("%s: classe %s" % (caminho, m.get_class().get_name()))
    try:
        L.append("  shading_model %s, blend %s" % (m.get_editor_property("shading_model"), m.get_editor_property("blend_mode")))
    except Exception as e:
        L.append("  ? %s" % e)
    for prop in ("MP_BASE_COLOR", "MP_EMISSIVE_COLOR", "MP_ROUGHNESS", "MP_METALLIC"):
        try:
            n = MEL.get_material_property_input_node(m, getattr(unreal.MaterialProperty, prop))
            desc = "nada"
            if n:
                desc = n.get_class().get_name()
                for p in ("constant", "r", "default_value", "parameter_name"):
                    try:
                        desc += " %s=%s" % (p, n.get_editor_property(p))
                    except Exception:
                        pass
            L.append("  %s <- %s" % (prop, desc))
        except Exception as e:
            L.append("  %s ? %s" % (prop, e))
    try:
        for e in MEL.get_num_material_expressions(m) and [] or []:
            pass
        L.append("  expressoes: %d" % MEL.get_num_material_expressions(m))
    except Exception as e:
        L.append("  expr ? %s" % e)
with open("C:/Backrooms/Backrooms/Scripts/resultado_material.txt", "w") as f:
    f.write("\n".join(L))
unreal.log_warning("[Mat] pronto")
