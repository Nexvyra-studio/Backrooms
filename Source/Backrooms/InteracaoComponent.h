#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "InteracaoComponent.generated.h"

class UInputAction;
class UInputMappingContext;

// Coloque no personagem. Aperte E olhando para algo interagivel (ex.: quadro de luz).
// Funciona no multiplayer: o pedido vai para o servidor, que executa a acao.
// Jeito certo: crie IA_Interagir + IMC_Interacao no editor e escolha aqui.
// Se ficarem vazios, o componente cria a tecla E sozinho (so para nao quebrar).
UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UInteracaoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteracaoComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Distancia maxima para alcancar algo (cm)
    UPROPERTY(EditAnywhere, Category = "Interacao")
    float Alcance = 220.f;

    // Input Action de interagir (ex.: IA_Interagir, tipo Digital/bool)
    UPROPERTY(EditAnywhere, Category = "Interacao")
    TObjectPtr<UInputAction> AcaoInteragir;

    // Mapping Context com a tecla da acao acima (ex.: IMC_Interacao, E -> IA_Interagir)
    UPROPERTY(EditAnywhere, Category = "Interacao")
    TObjectPtr<UInputMappingContext> Mapeamento;

    // Usada so se os dois campos acima estiverem vazios
    UPROPERTY(EditAnywhere, Category = "Interacao")
    FKey Tecla = EKeys::E;

private:
    void Interagir();
    AActor* ProcurarAlvo() const;
    void TentarLigarEntrada();

    UFUNCTION(Server, Reliable)
    void Server_Interagir(AActor* Alvo);

    bool bEntradaLigada = false;

    // O que o jogador local esta mirando agora (para mostrar a dica)
    TWeakObjectPtr<AActor> AlvoDestacado;
    void AtualizarDestaque();
};
