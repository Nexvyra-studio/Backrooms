import unreal, os
pasta = "C:/Backrooms/Backrooms/Audio/export"
os.makedirs(pasta, exist_ok=True)
L = []
itens = [("/Game/Sounds/Passo_Carpete_01", "Passo_Carpete_01.wav"),
         ("/Game/Sounds/Passo_Carpete_02", "Passo_Carpete_02.wav"),
         ("/Game/Sounds/Passo_Carpete_03", "Passo_Carpete_03.wav"),
         ("/Game/Textures/OfficeCeiling001_2K-JPG_Color", "teto.tga")]
for caminho, arquivo in itens:
    a = unreal.load_asset(caminho)
    t = unreal.AssetExportTask()
    t.set_editor_property("object", a)
    t.set_editor_property("filename", pasta + "/" + arquivo)
    t.set_editor_property("automated", True)
    t.set_editor_property("prompt", False)
    t.set_editor_property("replace_identical", True)
    ok = unreal.Exporter.run_asset_export_task(t)
    L.append("%s -> %s : %s" % (caminho, arquivo, ok))
    for src in ("source_file", ):
        pass
    try:
        info = a.get_editor_property("asset_import_data")
        L.append("   origem: %s" % info.get_first_filename())
    except Exception as e:
        L.append("   origem ? %s" % e)
# como o material mestre monta as UVs do mundo
m = unreal.load_asset("/Game/Materials/M_MestreMundo")
MEL = unreal.MaterialEditingLibrary
try:
    L.append("M_MestreMundo expressoes: %d" % MEL.get_num_material_expressions(m))
except Exception as e:
    L.append("? %s" % e)
with open("C:/Backrooms/Backrooms/Scripts/resultado_export.txt", "w") as f:
    f.write("\n".join(L))
unreal.log_warning("[Export] pronto")
