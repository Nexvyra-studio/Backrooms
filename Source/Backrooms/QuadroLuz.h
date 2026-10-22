#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interagivel.h"
#include "QuadroLuz.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UAudioComponent;
class USoundBase;
class UMaterialInterface;
class ALampada;

// Quadro de luz velho preso na parede. Desligar os quadros faz parte do enigma de saida.
// O gerador do Level 0 coloca dois grupos: perto um do outro (1-2 jogadores)
// e espalhados pelo mapa (3-5 jogadores). O enigma ativa so um grupo.
UCLASS()
class BACKROOMS_API AQuadroLuz : public AActor, public IInteragivel
{
    GENERATED_BODY()

public:
    AQuadroLuz();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // IInteragivel
    virtual void Interagir(APawn* Quem) override;
    virtual bool PodeInteragir() const override { return bAtivo && bLigado; }
    virtual void DefinirDestaque(bool bDestacado) override;

    // ---------- Preenchido pelo gerador ----------
    // 0 = grupo perto (1-2 jogadores), 1 = grupo espalhado (3-5 jogadores)
    UPROPERTY(EditAnywhere, Category = "Quadro")
    int32 Grupo = 0;

    // Rabisco na parede ao lado (ex.: I, II, III)
    UPROPERTY(EditAnywhere, Category = "Quadro")
    FText TextoRabisco;

    // Altura do teto, para o conduite subir ate o forro
    UPROPERTY(EditAnywhere, Category = "Quadro")
    float AlturaTeto = 270.f;

    // Quanto o conduite anda pelo forro (a pista "siga o fio no teto")
    UPROPERTY(EditAnywhere, Category = "Quadro")
    float ComprimentoConduite = 300.f;

    // Lampada logo acima: ela fica sem zumbido enquanto o quadro estiver ativo
    UPROPERTY(EditAnywhere, Category = "Quadro")
    TObjectPtr<ALampada> LampadaSilenciosa;

    // ---------- Visual (escolhido no BP_QuadroLuz) ----------
    // Material da caixa, alavanca, conduites e parafusos
    UPROPERTY(EditAnywhere, Category = "Quadro|Visual")
    TObjectPtr<UMaterialInterface> MaterialMetal;

    // ---------- Som (escolhido no BP_QuadroLuz; a atenuacao fica no componente "Chiado") ----------
    UPROPERTY(EditAnywhere, Category = "Quadro|Som")
    TObjectPtr<USoundBase> SomAlavanca;

    UPROPERTY(EditAnywhere, Category = "Quadro|Som")
    TObjectPtr<USoundBase> SomChiado;

    // ---------- Usado pelo enigma (servidor) ----------
    void Religar();
    void DefinirAtivo(bool bNovo);
    bool EstaLigado() const { return bLigado; }
    bool EstaAtivo() const { return bAtivo; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UStaticMeshComponent> Caixa;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<USceneComponent> Pivo;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UStaticMeshComponent> Alavanca;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UStaticMeshComponent> Indicador;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UStaticMeshComponent> ConduiteSubida;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UStaticMeshComponent> ConduiteTeto;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UTextRenderComponent> Etiqueta;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UTextRenderComponent> Rabisco;

    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UAudioComponent> Chiado;

    // Dica que aparece so quando o jogador mira na alavanca
    UPROPERTY(VisibleAnywhere, Category = "Quadro")
    TObjectPtr<UTextRenderComponent> Dica;

    // Luzinha do quadro (ligado/desligado): escolhidos no BP_QuadroLuz
    UPROPERTY(EditAnywhere, Category = "Quadro|Visual")
    TObjectPtr<UMaterialInterface> MaterialIndicadorLigado;

    UPROPERTY(EditAnywhere, Category = "Quadro|Visual")
    TObjectPtr<UMaterialInterface> MaterialIndicadorDesligado;

private:
    UPROPERTY(ReplicatedUsing = OnRep_Ligado)
    bool bLigado = true;

    UPROPERTY(ReplicatedUsing = OnRep_Ativo)
    bool bAtivo = true;

    UFUNCTION()
    void OnRep_Ligado();

    UFUNCTION()
    void OnRep_Ativo();

    void AtualizarAlavanca(bool bComSom);
    void AplicarAtivo();
};
