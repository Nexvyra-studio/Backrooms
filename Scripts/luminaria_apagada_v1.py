# Lampada apagada de verdade: difusor cinza claro SEM brilho (antes continuava brilhando no apagao)
#   py "C:/Backrooms/Backrooms/Scripts/luminaria_apagada_v1.py"
import unreal
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Materials/M_LuminariaApagada")
cor = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -500, 0)
cor.set_editor_property("constant", unreal.LinearColor(0.50, 0.50, 0.47, 1.0))
MEL.connect_material_property(cor, "", unreal.MaterialProperty.MP_BASE_COLOR)
zero = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -500, 200)
zero.set_editor_property("r", 0.0)
MEL.connect_material_property(zero, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rug = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -500, 300)
rug.set_editor_property("r", 0.6)
MEL.connect_material_property(rug, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
n = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.log_warning("[Mat] M_LuminariaApagada sem brilho: emissivo <- %s" % (n.get_class().get_name() if n else "nada"))
with open("C:/Backrooms/Backrooms/Scripts/resultado_material.txt", "w") as f:
    f.write("emissivo <- %s" % (n.get_class().get_name() if n else "nada"))
