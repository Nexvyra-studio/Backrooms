# Mostra onde estao os quadros de luz e a saida, com o comando para se teleportar (BugItGo)
#   py "C:/Backrooms/Backrooms/Scripts/listar_quadros.py"
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def comando(a, dist=120.0):
    loc = a.get_actor_location()
    f = a.get_actor_forward_vector()
    x = loc.x + f.x * dist
    y = loc.y + f.y * dist
    z = loc.z + 100.0
    # olhando para o objeto
    yaw = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(x, y, z), unreal.Vector(loc.x, loc.y, z)).yaw
    return "BugItGo %d %d %d 0 %d 0" % (x, y, z, yaw)

for a in sorted(eas.get_all_level_actors(), key=lambda a: a.get_actor_label()):
    n = a.get_class().get_name()
    if n == "QuadroLuz":
        g = a.get_editor_property("grupo")
        quem = "1-2 jogadores" if g == 0 else "3-5 jogadores"
        unreal.log_warning("[Mapa] Quadro (%s) %s  ->  %s" % (quem, a.get_actor_label(), comando(a)))
    elif n == "ParedeSaida":
        unreal.log_warning("[Mapa] SAIDA %s  ->  %s" % (a.get_actor_label(), comando(a, 250.0)))
