# Mostra (no editor) como a sombra fica, 6 m na frente da camera. Rode de novo com "apagar" para remover.
#   py "C:/Backrooms/Backrooms/Scripts/ver_sombra_v1.py"
#   py "C:/Backrooms/Backrooms/Scripts/ver_sombra_v1.py" apagar
import sys
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

for a in eas.get_all_level_actors():
    if a.get_actor_label() == "PreviaSombra":
        eas.destroy_actor(a)

if len(sys.argv) < 2 or sys.argv[1] != "apagar":
    loc, rot = ues.get_level_viewport_camera_info()
    frente = rot.get_forward_vector()
    alvo = unreal.Vector(loc.x + frente.x * 600.0, loc.y + frente.y * 600.0, 0.0)
    classe = unreal.load_class(None, "/Script/Backrooms.SombraCorredor")
    s = eas.spawn_actor_from_class(classe, alvo, unreal.Rotator(0, 0, rot.yaw + 180.0))
    s.set_actor_label("PreviaSombra")
    corpo = s.get_component_by_class(unreal.SkeletalMeshComponent)
    preto = unreal.load_asset("/Game/Materials/M_Sombra")
    for i in range(corpo.get_num_materials()):
        corpo.set_material(i, preto)
    unreal.log_warning("[Sombra] previa criada")
