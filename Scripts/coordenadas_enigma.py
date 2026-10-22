# Mostra no Output Log os comandos BugItGo para ir direto aos quadros e a saida
#   py "C:/Backrooms/Backrooms/Scripts/coordenadas_enigma.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def comando(ator, nome):
    frente = ator.get_actor_forward_vector()
    p = ator.get_actor_location()
    # 1,5 m na frente da parede, olhando para ela
    x = p.x + frente.x * 150.0
    y = p.y + frente.y * 150.0
    rot = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(x, y, 0), unreal.Vector(p.x, p.y, 0))
    unreal.log_warning("[Coord] %-22s BugItGo %d %d %d 0 %d 0" % (nome, x, y, p.z + 100, rot.yaw))

quadros = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "QuadroLuz"]
quadros.sort(key=lambda a: (a.get_editor_property("grupo"), a.get_name()))
for i, q in enumerate(quadros):
    g = q.get_editor_property("grupo")
    grupo = "solo/dupla" if g == 0 else "3-5 jogadores"
    comando(q, "Quadro %d (%s)" % (i % 3 + 1, grupo))

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "ParedeSaida":
        comando(a, "SAIDA")
