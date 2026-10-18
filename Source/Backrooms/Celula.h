#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Celula.generated.h"

class UStaticMeshComponent;
class UChildActorComponent;
class UMaterialInterface;
class UStaticMesh;

// Uma "célula" do labirinto: 4 m x 4 m, 3 m de altura.
// Piso + teto + lâmpada + até 4 paredes que você liga/desliga.
UCLASS()
class BACKROOMS_API ACelula : public AActor
{
    GENERATED_BODY()

public:
    ACelula();

    // Roda no EDITOR toda vez que você mexe na célula (move, marca uma caixinha...)
    virtual void OnConstruction(const FTransform& Transform) override;

    // ---------- Paredes (marque quais lados têm parede) ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celula|Paredes")
    bool bParedeNorte = true;   // +X (seta vermelha)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celula|Paredes")
    bool bParedeSul = true;     // -X

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celula|Paredes")
    bool bParedeLeste = true;   // +Y (seta verde)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celula|Paredes")
    bool bParedeOeste = true;   // -Y

    // ---------- Lâmpada ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celula|Lampada")
    bool bTemLampada = true;

    // Qual lâmpada usar (coloque o BP_LampadaCpp aqui)
    UPROPERTY(EditAnywhere, Category = "Celula|Lampada")
    TSubclassOf<AActor> ClasseLampada;

    // ---------- Materiais ----------
    UPROPERTY(EditAnywhere, Category = "Celula|Materiais")
    TObjectPtr<UMaterialInterface> MaterialParede;

    UPROPERTY(EditAnywhere, Category = "Celula|Materiais")
    TObjectPtr<UMaterialInterface> MaterialPiso;

    UPROPERTY(EditAnywhere, Category = "Celula|Materiais")
    TObjectPtr<UMaterialInterface> MaterialTeto;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> Piso;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> Teto;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> ParedeNorte;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> ParedeSul;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> ParedeLeste;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UStaticMeshComponent> ParedeOeste;

    UPROPERTY(VisibleAnywhere, Category = "Celula")
    TObjectPtr<UChildActorComponent> Lampada;

private:
    // Cria um cubo posicionado e escalado (usado só no construtor)
    UStaticMeshComponent* CriarPeca(const TCHAR* Nome, const FVector& Local, const FVector& TamanhoCm, UStaticMesh* Malha);
};