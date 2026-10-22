#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bilhete.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

// Folha de papel colada na parede, deixada por alguem que passou por aqui antes.
// O texto e lido chegando perto (nao precisa de interface nem tecla).
UCLASS()
class BACKROOMS_API ABilhete : public AActor
{
    GENERATED_BODY()

public:
    ABilhete();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, Category = "Bilhete", meta = (MultiLine = "true"))
    FText Texto;

    // Se preenchido: depois que voce le e vai embora, o texto do papel MUDA.
    // (Volte nele e veja.) Acontece so uma vez, so para quem leu.
    UPROPERTY(EditAnywhere, Category = "Bilhete", meta = (MultiLine = "true"))
    FText TextoDepois;

    virtual void Tick(float DeltaSeconds) override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Bilhete")
    TObjectPtr<UStaticMeshComponent> Papel;

    UPROPERTY(VisibleAnywhere, Category = "Bilhete")
    TObjectPtr<UTextRenderComponent> Escrita;

private:
    bool bLido = false;
    bool bMudou = false;
    float TempoLendo = 0.f;
};
