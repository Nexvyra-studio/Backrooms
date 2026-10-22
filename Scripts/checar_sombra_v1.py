import unreal
som = "/Game/Sounds/Susto_Sombra"
if unreal.EditorAssetLibrary.does_asset_exist(som):
    unreal.EditorAssetLibrary.save_asset(som)
    unreal.log_warning("[Sombra] som salvo")
c = unreal.load_class(None, "/Script/Backrooms.SombraCorredor")
unreal.log_warning("[Sombra] classe SombraCorredor: %s" % ("COMPILADA" if c else "NAO COMPILADA"))
