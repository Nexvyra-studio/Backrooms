# Cria o material preto puro da sombra:  py "C:/Backrooms/Backrooms/Scripts/criar_material_sombra_v1.py"
import unreal

caminho = "/Game/Materials/M_Sombra"
if unreal.EditorAssetLibrary.does_asset_exist(caminho):
    unreal.log_warning("[Sombra] M_Sombra ja existe")
else:
    mel = unreal.MaterialEditingLibrary
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_Sombra", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    c = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    c.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(caminho)
    unreal.log_warning("[Sombra] M_Sombra criado")

som = "/Game/Sounds/Susto_Sombra"
unreal.log_warning("[Sombra] som Susto_Sombra: %s" % ("ok" if unreal.EditorAssetLibrary.does_asset_exist(som) else "AINDA NAO IMPORTADO"))
