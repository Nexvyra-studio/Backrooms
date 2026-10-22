#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LuzEmergencia.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

// Setinha verde de emergencia no teto. Acende no apagao do enigma e aponta o caminho ate a SAIDA.
// (Como em predio de verdade: quando a energia cai, as luzes de emergencia mostram a rota de fuga.)
UCLASS()
class BACKROOMS_API ALuzEmergencia : public AActor
{
    GENERATED_BODY()

public:
    ALuzEmergencia();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Emergencia")
    TObjectPtr<UStaticMeshComponent> BracoEsquerdo;

    UPROPERTY(VisibleAnywhere, Category = "Emergencia")
    TObjectPtr<UStaticMeshComponent> BracoDireito;

    UPROPERTY(VisibleAnywhere, Category = "Emergencia")
    TObjectPtr<UPointLightComponent> Luz;

private:
    float Tempo = 0.f;
    float Fase = 0.f;
};
