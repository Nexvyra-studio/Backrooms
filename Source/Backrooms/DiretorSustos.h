#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "DiretorSustos.generated.h"

class ALampada;
class ASombraCorredor;
class APawn;
class USoundBase;
class USoundAttenuation;

UENUM(BlueprintType)
enum class ETipoSusto : uint8
{
    Apagao,    // as luzes apagam em cascata, do fundo do corredor ate o jogador
    Passos,    // passos atras do jogador; param no instante em que ele se vira
    Silencio,  // o zumbido de todas as lampadas some de uma vez
    Piscada,   // todas as lampadas em volta piscam juntas, como uma falha na rede
    Batidas    // tres batidas secas DENTRO da parede ao lado do jogador
};

// Diretor de Sustos: o medo vem do AMBIENTE, nao de monstros.
// Roda no servidor e escolhe quando e qual susto acontece; os efeitos sao enviados
// para todos os jogadores (multiplayer: todo mundo ve e ouve a mesma coisa).
// O gerador do Level 0 ja coloca um no mapa. Ele tambem pode ser arrastado manualmente.
UCLASS()
class BACKROOMS_API ADiretorSustos : public AActor
{
    GENERATED_BODY()

public:
    ADiretorSustos();

    // ================= Ritmo =================
    // O primeiro susto acontece X segundos depois do PRIMEIRO PASSO de qualquer jogador
    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo", meta = (ClampMin = "0.0"))
    float AtrasoAposPrimeiroPasso = 90.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo")
    ETipoSusto PrimeiroSusto = ETipoSusto::Apagao;

    // Tempo entre um susto e outro (segundos)
    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo")
    FVector2D IntervaloEntre = FVector2D(100.f, 180.f);

    // A cada susto o intervalo encolhe um pouco (a tensao cresce com o tempo)
    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo", meta = (ClampMin = "0.5", ClampMax = "1.0"))
    float AceleracaoTensao = 0.97f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo", meta = (ClampMin = "5.0"))
    float IntervaloMinimo = 70.f;

    // Para testar: um susto a cada 6-10 segundos
    UPROPERTY(EditAnywhere, Category = "Sustos|Ritmo")
    bool bModoTeste = false;

    // ================= Chance de cada susto =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Pesos", meta = (ClampMin = "0.0"))
    float PesoApagao = 3.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Pesos", meta = (ClampMin = "0.0"))
    float PesoPassos = 3.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Pesos", meta = (ClampMin = "0.0"))
    float PesoSilencio = 2.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Pesos", meta = (ClampMin = "0.0"))
    float PesoPiscada = 2.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Pesos", meta = (ClampMin = "0.0"))
    float PesoBatidas = 2.f;

    // ================= Batidas =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Batidas")
    TObjectPtr<USoundBase> SomBatidas;

    UPROPERTY(EditAnywhere, Category = "Sustos|Batidas", meta = (ClampMin = "0.0"))
    float VolumeBatidas = 1.6f;

    // ================= Apagao =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Apagao")
    float RaioApagao = 2200.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Apagao")
    FVector2D DuracaoApagao = FVector2D(6.f, 10.f);

    // Tempo entre uma lampada e a proxima apagar (a escuridao "vem andando")
    UPROPERTY(EditAnywhere, Category = "Sustos|Apagao")
    float TempoCascata = 0.06f;

    // ================= Sombra (aparicao depois do apagao) =================
    // Som curto e assustador quando a sombra passa (escolhido no BP_DiretorSustos)
    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra")
    TObjectPtr<USoundBase> SomSombra;

    // Qual ator aparece como sombra (use o BP_Sombra para trocar malha, material e animacao)
    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra")
    TSubclassOf<ASombraCorredor> ClasseSombra;

    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra", meta = (ClampMin = "0.0"))
    float VolumeSombra = 1.2f;

    // Depois do primeiro apagao ela SEMPRE aparece; nos seguintes, com esta chance
    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceSombra = 0.25f;

    // Nas vezes seguintes: chance de ser o "vulto parado" no fundo do corredor em vez da travessia
    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceVulto = 0.4f;

    // Velocidade com que ela atravessa o corredor (cm/s)
    UPROPERTY(EditAnywhere, Category = "Sustos|Sombra")
    float VelocidadeSombra = 750.f;

    // ================= Passos =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    TArray<TObjectPtr<USoundBase>> SonsPassos;

    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    TObjectPtr<USoundAttenuation> AtenuacaoPassos;

    // A que distancia atras do jogador os passos comecam (cm)
    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    FVector2D DistanciaPassos = FVector2D(450.f, 750.f);

    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    FIntPoint NumeroPassos = FIntPoint(10, 16);

    // Ritmo de alguem andando devagar
    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    float IntervaloPassos = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Passos")
    float ComprimentoPasso = 75.f;

    UPROPERTY(EditAnywhere, Category = "Sustos|Passos", meta = (ClampMin = "0.0"))
    float VolumePassos = 1.1f;

    // ================= Silencio =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Silencio")
    FVector2D DuracaoSilencio = FVector2D(10.f, 16.f);

    // ================= Piscada =================
    UPROPERTY(EditAnywhere, Category = "Sustos|Piscada")
    float RaioPiscada = 3000.f;

    // Dispara um susto na hora (Blueprint ou testes)
    UFUNCTION(BlueprintCallable, Category = "Sustos")
    void DispararSusto(ETipoSusto Tipo);

    // Pausa os sustos (ex.: durante o apagao do enigma)
    UFUNCTION(BlueprintCallable, Category = "Sustos")
    void DefinirPausado(bool bNovo);

protected:
    virtual void BeginPlay() override;

private:
    void Agendar(float Atraso);
    void VerificarPrimeiroPasso();
    void Proximo();
    ETipoSusto Sortear() const;
    APawn* EscolherAlvo() const;

    void IniciarPassos(APawn* Alvo);
    void DarPasso();

    TArray<ALampada*> LampadasPerto(const FVector& Centro, float Raio) const;

    // Sombra: roda em cada maquina, para o jogador local
    void TentarSombra();
    bool CriarSombra();
    bool CriarVulto(APawn* Eu, const FVector& Olho, const FVector& Frente, float Profundidade, float Chao);
    void EfeitosTravessia(const FVector& De, const FVector& Para, float Duracao, APawn* Eu);
    void AgendarLocal(float Atraso, TFunction<void()> Acao);
    bool bSombraPrimeira = false;
    FTimerHandle TimerSombra;
    int32 TentativasSombra = 0;
    bool bJaTeveSombra = false;
    void AgendarLampada(ALampada* Lampada, float Atraso, bool bApagar, bool bComEstalo);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Apagao(FVector_NetQuantize Centro, float Duracao, bool bComSombra, bool bPrimeiraSombra);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Piscada(FVector_NetQuantize Centro);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Silencio(FVector_NetQuantize Centro, float Duracao);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_Passo(FVector_NetQuantize Local);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Batidas(FVector_NetQuantize Local);

    bool AcharParedeAoLado(APawn* Alvo, FVector& Saida) const;

    FTimerHandle TimerProximo;
    FTimerHandle TimerPassos;
    FTimerHandle TimerEsperaPasso;
    bool bPausado = false;
    float TempoPausado = 0.f;

    bool bPrimeiro = true;
    ETipoSusto Ultimo = ETipoSusto::Piscada;
    float FatorTensao = 1.f;

    TWeakObjectPtr<APawn> AlvoPassos;
    FVector PosicaoPassos = FVector::ZeroVector;
    int32 PassosRestantes = 0;
};
