#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalancoCameraComponent.generated.h"

class UCameraComponent;
class UCorridaComponent;
class UPassosComponent;

// Deixa a camera "viva": balanca ao andar, mais forte ao correr, respira parada,
// ofega quando o folego acaba e afunda um pouco ao pousar de um pulo.
// Roda so na maquina de quem joga (nao pesa na rede).
UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UBalancoCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBalancoCameraComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ---------- Andar / correr ----------
    // Altura do balanco ao andar (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float AlturaAndar = 1.1f;

    // Altura do balanco ao correr (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float AlturaCorrer = 2.4f;

    // Balanco de um lado para o outro (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float Lateral = 0.7f;

    // Inclinacao da camera quando anda de lado (graus)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float InclinacaoLateral = 1.2f;

    // ---------- Parado ----------
    // Respiracao parado (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float Respiracao = 0.35f;

    // Ofegar quando o folego acaba (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float Ofego = 0.9f;

    // Afundada ao pousar (cm)
    UPROPERTY(EditAnywhere, Category = "Balanco")
    float AfundarPouso = 6.f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY()
    TObjectPtr<UCorridaComponent> Corrida;

    // Os passos ditam o ritmo do balanco (o pe toca o chao = camera no ponto mais baixo)
    UPROPERTY()
    TObjectPtr<UPassosComponent> Passos;

    FVector OffsetAnterior = FVector::ZeroVector;
    float RollAnterior = 0.f;
    float Fase = 0.f;
    float FaseRespiracao = 0.f;
    float Cansaco = 0.f;
    float Intensidade = 0.f;
    float Pouso = 0.f;
    float RollAtual = 0.f;
    bool bEstavaNoAr = false;
};
