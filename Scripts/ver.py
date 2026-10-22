# Coloca a camera do editor na frente de um objeto do enigma.
#   py "C:/Backrooms/Backrooms/Scripts/ver.py" Bilhete 0
#   py "C:/Backrooms/Backrooms/Scripts/ver.py" QuadroLuz 0
#   py "C:/Backrooms/Backrooms/Scripts/ver.py" ParedeSaida 0
import sys
import unreal

classe = sys.argv[1] if len(sys.argv) > 1 else "Bilhete"
indice = int(sys.argv[2]) if len(sys.argv) > 2 else 0
distancia = float(sys.argv[3]) if len(sys.argv) > 3 else 140.0

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

lista = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == classe]
lista.sort(key=lambda a: a.get_name())
if not lista:
    unreal.log_warning("[Ver] nenhum " + classe)
else:
    alvo = lista[indice % len(lista)]
    frente = alvo.get_actor_forward_vector()
    base = alvo.get_actor_location()
    altura = 145.0 if classe == "Bilhete" else 170.0
    olho = unreal.Vector(base.x + frente.x * distancia, base.y + frente.y * distancia, base.z + altura)
    alvo_ponto = unreal.Vector(base.x, base.y, base.z + altura - 10.0)
    rot = unreal.MathLibrary.find_look_at_rotation(olho, alvo_ponto)
    ues.set_level_viewport_camera_info(olho, rot)
    unreal.log_warning("[Ver] %s %d de %d: %s" % (classe, indice % len(lista), len(lista), alvo.get_actor_label()))
