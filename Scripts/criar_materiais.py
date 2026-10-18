# ============================================================
#  Backrooms - cria o material mestre "alinhado ao mundo"
#  e as Material Instances (MI_Parede, MI_Carpete, MI_Teto).
#
#  Como rodar: na Unreal, Tools > Execute Python Script...
#  e escolha este arquivo. Pode rodar quantas vezes quiser:
#  ele recria/atualiza tudo.
# ============================================================
import unreal

AT  = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

PASTA_MATERIAIS = "/Game/Materials"
PASTA_TEXTURAS  = "/Game/Textures"
NOME_MESTRE     = "M_MestreMundo"

FUNCAO_TEXTURA = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture"
FUNCAO_NORMAL  = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal.WorldAlignedNormal"


def log(msg):
    unreal.log("[Backrooms] " + msg)


def achar_textura(*partes):
    """Procura na pasta Textures uma textura cujo nome contenha todas as 'partes'."""
    for caminho in EAL.list_assets(PASTA_TEXTURAS, recursive=True):
        nome = caminho.split("/")[-1].split(".")[0].lower()
        if all(p.lower() in nome for p in partes):
            return unreal.load_asset(caminho)
    unreal.log_warning("[Backrooms] Textura nao encontrada: " + " + ".join(partes))
    return None


def carregar_ou_criar(nome, classe, fabrica):
    caminho = PASTA_MATERIAIS + "/" + nome
    if EAL.does_asset_exist(caminho):
        return unreal.load_asset(caminho)
    return AT.create_asset(nome, PASTA_MATERIAIS, classe, fabrica)


def novo_no(material, classe, x, y):
    return MEL.create_material_expression(material, classe, x, y)


# ------------------------------------------------------------
# 1) Material mestre
# ------------------------------------------------------------
def criar_mestre(textura_cor_padrao, textura_normal_padrao):
    mestre = carregar_ou_criar(NOME_MESTRE, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mestre)   # recomeça do zero

    funcao_tex = unreal.load_asset(FUNCAO_TEXTURA)
    funcao_nor = unreal.load_asset(FUNCAO_NORMAL)

    # Parâmetros (viram campos nas Material Instances)
    tamanho = novo_no(mestre, unreal.MaterialExpressionScalarParameter, -900, 0)
    tamanho.set_editor_property("parameter_name", "TamanhoTextura")
    tamanho.set_editor_property("default_value", 100.0)

    tex_cor = novo_no(mestre, unreal.MaterialExpressionTextureObjectParameter, -900, -300)
    tex_cor.set_editor_property("parameter_name", "TexturaCor")
    if textura_cor_padrao:
        tex_cor.set_editor_property("texture", textura_cor_padrao)

    tex_nor = novo_no(mestre, unreal.MaterialExpressionTextureObjectParameter, -900, 300)
    tex_nor.set_editor_property("parameter_name", "TexturaNormal")
    if textura_normal_padrao:
        tex_nor.set_editor_property("texture", textura_normal_padrao)
    tex_nor.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)

    tinta = novo_no(mestre, unreal.MaterialExpressionVectorParameter, -500, -500)
    tinta.set_editor_property("parameter_name", "Tinta")
    tinta.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))

    rugosidade = novo_no(mestre, unreal.MaterialExpressionScalarParameter, -300, 200)
    rugosidade.set_editor_property("parameter_name", "Rugosidade")
    rugosidade.set_editor_property("default_value", 0.8)

    # Cor alinhada ao mundo x Tinta
    wa_cor = novo_no(mestre, unreal.MaterialExpressionMaterialFunctionCall, -500, -250)
    wa_cor.set_editor_property("material_function", funcao_tex)
    MEL.connect_material_expressions(tex_cor, "", wa_cor, "TextureObject")
    MEL.connect_material_expressions(tamanho, "", wa_cor, "TextureSize")

    multiplica = novo_no(mestre, unreal.MaterialExpressionMultiply, -200, -300)
    MEL.connect_material_expressions(wa_cor, "XYZ Texture", multiplica, "A")
    MEL.connect_material_expressions(tinta, "", multiplica, "B")
    MEL.connect_material_property(multiplica, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Relevo alinhado ao mundo
    wa_nor = novo_no(mestre, unreal.MaterialExpressionMaterialFunctionCall, -500, 300)
    wa_nor.set_editor_property("material_function", funcao_nor)
    MEL.connect_material_expressions(tex_nor, "", wa_nor, "TextureObject")
    MEL.connect_material_expressions(tamanho, "", wa_nor, "TextureSize")
    MEL.connect_material_property(wa_nor, "XYZ Texture", unreal.MaterialProperty.MP_NORMAL)

    # Rugosidade (um número só, fosco)
    MEL.connect_material_property(rugosidade, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.layout_material_expressions(mestre)
    MEL.recompile_material(mestre)
    EAL.save_asset(PASTA_MATERIAIS + "/" + NOME_MESTRE)
    log("Material mestre criado: " + NOME_MESTRE)
    return mestre


# ------------------------------------------------------------
# 2) Material Instances
# ------------------------------------------------------------
def criar_instancia(nome, mestre, cor, normal, tamanho_cm, tinta, rugosidade):
    mi = carregar_ou_criar(nome, unreal.MaterialInstanceConstant,
                           unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, mestre)
    if cor:
        MEL.set_material_instance_texture_parameter_value(mi, "TexturaCor", cor)
    if normal:
        MEL.set_material_instance_texture_parameter_value(mi, "TexturaNormal", normal)
    MEL.set_material_instance_scalar_parameter_value(mi, "TamanhoTextura", tamanho_cm)
    MEL.set_material_instance_scalar_parameter_value(mi, "Rugosidade", rugosidade)
    MEL.set_material_instance_vector_parameter_value(mi, "Tinta", tinta)
    MEL.update_material_instance(mi)
    EAL.save_asset(PASTA_MATERIAIS + "/" + nome)
    log("Material Instance criada: " + nome)


def main():
    parede_cor   = achar_textura("wallpaper", "color")
    parede_nor   = achar_textura("wallpaper", "normaldx")
    carpete_cor  = achar_textura("carpet", "color")
    carpete_nor  = achar_textura("carpet", "normaldx")
    teto_cor     = achar_textura("ceiling", "color")
    teto_nor     = achar_textura("ceiling", "normaldx")

    mestre = criar_mestre(parede_cor, parede_nor)

    #                nome          cor          normal       tamanho  tinta (R, G, B)                              rugosidade
    criar_instancia("MI_Parede",  mestre, parede_cor,  parede_nor,  100.0, unreal.LinearColor(0.584, 0.456, 0.098, 1), 0.8)
    criar_instancia("MI_Carpete", mestre, carpete_cor, carpete_nor,  50.0, unreal.LinearColor(1.0, 1.0, 1.0, 1),       0.9)
    criar_instancia("MI_Teto",    mestre, teto_cor,    teto_nor,    240.0, unreal.LinearColor(1.0, 1.0, 1.0, 1),       0.7)

    log("Pronto! Use MI_Parede, MI_Carpete e MI_Teto no BP_Celula.")


main()
