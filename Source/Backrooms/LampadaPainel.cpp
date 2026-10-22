#include "LampadaPainel.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

ALampadaPainel::ALampadaPainel()
{
    // Painel de 120 x 60 cm, 2 cm de espessura, rente ao forro
    Placa->SetRelativeLocation(FVector::ZeroVector);
    Placa->SetRelativeScale3D(FVector(1.2f, 0.6f, 0.02f));
    Placa->SetCastShadow(false);
    SlotsLuz = { 0 };

    // Luz larga e uniforme (difusor leitoso): espalha para os lados, sem "poca" de luz
    IntensidadeAcesa = 32.f;
    Luz->SetRelativeLocation(FVector(0.f, 0.f, -3.f));
    Luz->Intensity = IntensidadeAcesa;
    Luz->SourceWidth = 50.f;
    Luz->SourceHeight = 110.f;
    Luz->BarnDoorAngle = 88.f;
    Luz->BarnDoorLength = 20.f;
    Luz->AttenuationRadius = 520.f;
    Luz->IndirectLightingIntensity = 0.6f;

    // Desempenho: luzes longe da camera somem suavemente
    Luz->MaxDrawDistance = 3000.f;
    Luz->MaxDistanceFadeRange = 700.f;

    // Materiais (aceso/apagado), zumbido, estalos e atenuacao: escolhidos no BP_LampadaPainel

    // Zumbido mais baixo: no Level 0 sao muitas lampadas juntas
    VolumeZumbido = 0.35f;
}
