#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "EnigmaLevel0.generated.h"

class AQuadroLuz;
class AParedeSaida;
class ALuzEmergencia;
class ARabisco;
class APawn;
class USoundBase;
class UWorld;

UENUM(BlueprintType)
enum class EFaseEnigma : uint8
{
    Procurando,   // achar e desligar os quadros de luz
    Escuro,       // tudo apagado: a placa de SAIDA aparece
    Concluido     // alguem atravessou: indo para o proximo nivel
};

// Controla o enigma de saida do Level 0 (roda no servidor).
// 1) Desligar os 3 quadros de luz dentro do prazo (prazo e distancia mudam com o numero de jogadores).
// 2) Tudo apaga: aparece a placa de SAIDA.
// 3) A passagem so abre para quem chega de costas.
// Se o grupo empacar, o proprio lugar deixa rabiscos novos com setas.
UCLASS()
class BACKROOMS_API AEnigmaLevel0 : public AActor
{
    GENERATED_BODY()

public:
    AEnigmaLevel0();

    // ================= Prazo para desligar todos os quadros (s) =================
    UPROPERTY(EditAnywhere, Category = "Enigma|Prazo")
    float PrazoSolo = 180.f;

    UPROPERTY(EditAnywhere, Category = "Enigma|Prazo")
    float PrazoDupla = 100.f;

    UPROPERTY(EditAnywhere, Category = "Enigma|Prazo")
    float PrazoGrupo = 45.f;

    // ================= Escuro =================
    // Quanto tempo dura o apagao (depois tudo religa e precisa recomecar)
    UPROPERTY(EditAnywhere, Category = "Enigma|Escuro")
    float DuracaoEscuroSolo = 150.f;

    UPROPERTY(EditAnywhere, Category = "Enigma|Escuro")
    float DuracaoEscuroGrupo = 100.f;

    // ================= Dicas quando empaca (s) =================
    UPROPERTY(EditAnywhere, Category = "Enigma|Dicas")
    float TempoPrimeiraDicaSolo = 360.f;

    UPROPERTY(EditAnywhere, Category = "Enigma|Dicas")
    float TempoPrimeiraDicaGrupo = 480.f;

    UPROPERTY(EditAnywhere, Category = "Enigma|Dicas")
    float IntervaloDicas = 180.f;

    // ================= Saida =================
    // Mapa para onde o grupo vai ao sair (escolha o Lvl_Level1 no BP_EnigmaLevel0)
    UPROPERTY(EditAnywhere, Category = "Enigma")
    TSoftObjectPtr<UWorld> MapaSeguinte;

    // Escolhido no Blueprint (BP_EnigmaLevel0)
    UPROPERTY(EditAnywhere, Category = "Enigma")
    TObjectPtr<USoundBase> SomEnergia;

    // Setas verdes da rota de fuga (troque pelo BP_LuzEmergencia)
    UPROPERTY(EditAnywhere, Category = "Enigma|Classes")
    TSubclassOf<ALuzEmergencia> ClasseSeta;

    // Rabiscos de dica nas paredes (troque pelo BP_Rabisco)
    UPROPERTY(EditAnywhere, Category = "Enigma|Classes")
    TSubclassOf<ARabisco> ClasseRabisco;

    // Para testar rapido: prazo e dicas bem curtos
    UPROPERTY(EditAnywhere, Category = "Enigma")
    bool bModoTeste = false;

    // ---------- Chamado pelos quadros e pela parede (servidor) ----------
    void QuadroDesligado(AQuadroLuz* Quadro);
    void Concluir(APawn* Quem);

    EFaseEnigma GetFase() const { return Fase; }

    // ---------- Preenchido pelo gerador: rota de fuga ate a saida ----------
    // Para cada celula, qual e a proxima celula no caminho ate a saida (-1 = e a saida)
    UPROPERTY()
    TArray<int32> CaminhoSaida;

    UPROPERTY()
    int32 MapaColunas = 0;

    UPROPERTY()
    int32 MapaLinhas = 0;

    UPROPERTY()
    float MapaCelula = 360.f;

    UPROPERTY()
    float MapaAltura = 270.f;

    UPROPERTY()
    FTransform MapaOrigem;

    // De quantas em quantas celulas aparece uma seta de emergencia
    UPROPERTY(EditAnywhere, Category = "Enigma|Escuro", meta = (ClampMin = "1"))
    int32 EspacoSetas = 2;

protected:
    virtual void BeginPlay() override;

private:
    int32 NumeroJogadores() const;
    void EscolherGrupo();
    void PrazoEsgotado();
    void EntrarEscuro();
    void SairEscuro();
    void ReligarTodos();
    void DarDica();
    void Viajar();
    void PausarSustos(bool bPausar);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Energia(bool bLigada, bool bComRota);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Rabisco(FVector_NetQuantize Local, FRotator Rotacao, const FString& Texto);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Final();

    // Setas verdes de emergencia, do jogador ate a saida (cada maquina calcula a do seu jogador)
    void MostrarRotaDeFuga(bool bMostrar);

    UPROPERTY()
    TArray<TObjectPtr<ALuzEmergencia>> SetasEmergencia;

    UPROPERTY()
    TArray<TObjectPtr<AQuadroLuz>> Quadros;

    UPROPERTY()
    TObjectPtr<AParedeSaida> Parede;

    EFaseEnigma Fase = EFaseEnigma::Procurando;
    bool bAlgumProgresso = false;

    FTimerHandle TimerPrazo;
    FTimerHandle TimerEscuro;
    FTimerHandle TimerDica;
    FTimerHandle TimerViagem;
};
