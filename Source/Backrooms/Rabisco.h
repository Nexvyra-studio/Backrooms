#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rabisco.generated.h"

class UTextRenderComponent;

// Escrita de caneta na parede (pistas, setas, avisos).
UCLASS()
class BACKROOMS_API ARabisco : public AActor
{
    GENERATED_BODY()

public:
    ARabisco();

    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, Category = "Rabisco", meta = (MultiLine = "true"))
    FText Texto;

    UPROPERTY(EditAnywhere, Category = "Rabisco")
    float Tamanho = 14.f;

    UPROPERTY(EditAnywhere, Category = "Rabisco")
    FColor Cor = FColor(25, 25, 45);

    void Configurar(const FText& NovoTexto, float NovoTamanho, FColor NovaCor);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Rabisco")
    TObjectPtr<UTextRenderComponent> Escrita;
};
