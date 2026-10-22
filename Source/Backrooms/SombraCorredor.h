#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SombraCorredor.generated.h"

class USkeletalMeshComponent;

// Aparicao: uma silhueta preta que atravessa o fundo do corredor e some.
// Nao e um monstro: nao tem colisao, nao persegue, nao encosta em ninguem.
// Existe so na tela de UM jogador (cada um ve a sua... ou nao ve nada).
UCLASS()
class BACKROOMS_API ASombraCorredor : public AActor
{
    GENERATED_BODY()

public:
    ASombraCorredor();

    virtual void Tick(float DeltaSeconds) override;

    // Atravessa de De ate Para (no chao) em Duracao segundos e desaparece
    void Iniciar(const FVector& De, const FVector& Para, float Duracao);

    // Fica parado olhando para o jogador por Duracao segundos e some
    void IniciarParado(const FVector& Local, const FVector& OlharPara, float Duracao);

protected:
    // Malha (Skeletal Mesh) e tamanho: escolhidos no BP_Sombra, no componente "Corpo"
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sombra")
    TObjectPtr<USkeletalMeshComponent> Corpo;

    // Animacao quando atravessa o corredor
    UPROPERTY(EditAnywhere, Category = "Sombra")
    TObjectPtr<class UAnimSequence> AnimAndando;

    UPROPERTY(EditAnywhere, Category = "Sombra", meta = (ClampMin = "0.05"))
    float VelocidadeAnimAndando = 1.6f;

    // Animacao quando fica parada no fim do corredor
    UPROPERTY(EditAnywhere, Category = "Sombra")
    TObjectPtr<class UAnimSequence> AnimParado;

    UPROPERTY(EditAnywhere, Category = "Sombra", meta = (ClampMin = "0.05"))
    float VelocidadeAnimParado = 0.35f;

    // Material aplicado em todo o corpo (vazio = usa os materiais da propria malha)
    UPROPERTY(EditAnywhere, Category = "Sombra")
    TObjectPtr<class UMaterialInterface> MaterialSombra;

private:
    void PintarDePreto();

    FVector Inicio = FVector::ZeroVector;
    FVector Fim = FVector::ZeroVector;
    float Duracao = 0.4f;
    float Tempo = 0.f;
    bool bAndando = false;
    bool bParado = false;
};
