#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeradorLabirinto.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ALampada;

// Gera o nivel a partir de uma semente (mesma semente = mesmo mapa, importante no multiplayer).
//
// Como funciona:
//  1) Divisao recursiva: o espaco e cortado por paredes longas, como um escritorio
//     reformado sem logica. Cada parede ganha pelo menos um vao (porta/passagem).
//  2) Saloes abertos com pilares e paredes soltas que terminam no nada.
//  3) Corredores longos de portas alinhadas que somem na escuridao.
//  4) Zonas escuras com lampadas morrendo (piscando) e lampadas queimadas.
//
// Toda a geometria usa Instanced Static Meshes: poucos draw calls, aguenta mapas grandes.
UCLASS()
class BACKROOMS_API AGeradorLabirinto : public AActor
{
    GENERATED_BODY()

public:
    AGeradorLabirinto();

    // ================= Mapa =================
    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "2", ClampMax = "80"))
    int32 Colunas = 20;

    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "2", ClampMax = "80"))
    int32 Linhas = 20;

    // O "codigo" do mapa: mesmo numero = mesmo mapa
    UPROPERTY(EditAnywhere, Category = "Gerador")
    int32 Semente = 1234;

    // ================= Salas (divisao recursiva) =================
    // Menor lado de uma sala, em celulas (1 celula = 4 m)
    UPROPERTY(EditAnywhere, Category = "Gerador|Salas", meta = (ClampMin = "1"))
    int32 TamanhoMinimoSala = 2;

    // Areas maiores que isso sempre sao divididas
    UPROPERTY(EditAnywhere, Category = "Gerador|Salas", meta = (ClampMin = "2"))
    int32 TamanhoMaximoSala = 5;

    // Chance de parar de dividir (deixa uma sala grande inteira)
    UPROPERTY(EditAnywhere, Category = "Gerador|Salas", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceSalaGrande = 0.35f;

    // Chance de cada trecho de parede ganhar um vao a mais (menos becos sem saida)
    UPROPERTY(EditAnywhere, Category = "Gerador|Salas", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePortaExtra = 0.05f;

    // Chance de uma parede solida virar porta emparedada (vao fechado, "porta para lugar nenhum")
    UPROPERTY(EditAnywhere, Category = "Gerador|Salas", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePortaFalsa = 0.02f;

    // ================= Vaos =================
    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos")
    float LarguraPorta = 110.f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos")
    float AlturaPorta = 215.f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos")
    float LarguraPassagem = 220.f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos")
    float AlturaPassagem = 245.f;

    // Do total de vaos: quanto vira passagem larga (o resto vira porta)
    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePassagem = 0.45f;

    // Do total de vaos: quanto some por completo (parede inteira aberta)
    UPROPERTY(EditAnywhere, Category = "Gerador|Vaos", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceAbertaTotal = 0.2f;

    // ================= Saloes =================
    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "0"))
    int32 NumeroDeSaloes = 2;

    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "2"))
    int32 TamanhoSalaoMin = 3;

    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "2"))
    int32 TamanhoSalaoMax = 6;

    // Paredes isoladas no meio do salao, que comecam e terminam no nada
    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceParedeSolta = 0.02f;

    // Chance de pilar em cada canto aberto DENTRO dos saloes (fora deles nao ha pilares)
    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePilar = 0.25f;

    // ================= Corredores longos =================
    // Linhas retas de portas alinhadas: voce enxerga longe, mas nao o fim
    UPROPERTY(EditAnywhere, Category = "Gerador|Corredores", meta = (ClampMin = "0"))
    int32 NumeroCorredoresLongos = 3;

    UPROPERTY(EditAnywhere, Category = "Gerador|Corredores", meta = (ClampMin = "2"))
    int32 ComprimentoCorredorMin = 6;

    UPROPERTY(EditAnywhere, Category = "Gerador|Corredores", meta = (ClampMin = "2"))
    int32 ComprimentoCorredorMax = 16;

    // Quanto a luz some ao longo do corredor (0 = nada, 1 = fim totalmente escuro)
    UPROPERTY(EditAnywhere, Category = "Gerador|Corredores", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EscurecimentoCorredor = 0.9f;

    // ================= Luz e escuridao =================
    // Chance de lampada acesa nas areas normais
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampada = 0.3f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0"))
    int32 NumeroZonasEscuras = 3;

    // Raio de cada zona, em celulas (escurece aos poucos ate o centro)
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "1.0"))
    float RaioZonaEscura = 4.f;

    // Abaixo desse nivel de luz, as lampadas acesas comecam a falhar
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LimiteLampadaMorrendo = 0.45f;

    // Das lampadas perto do escuro, quantas realmente estao morrendo (piscando)
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampadaMorrendo = 0.3f;

    // Frequencia das piscadas das lampadas que estao morrendo
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceRajadaMorrendo = 0.03f;

    // No escuro, chance de existir uma luminaria queimada (apagada) no teto
    UPROPERTY(EditAnywhere, Category = "Gerador|Escuridao", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampadaQueimada = 0.3f;

    // ================= Visual =================
    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TSubclassOf<ALampada> ClasseLampada;

    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TObjectPtr<UMaterialInterface> MaterialParede;

    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TObjectPtr<UMaterialInterface> MaterialPiso;

    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TObjectPtr<UMaterialInterface> MaterialTeto;

    // Vazio = usa o material da parede
    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TObjectPtr<UMaterialInterface> MaterialPilar;

    // Vazio = usa o material da parede
    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    TObjectPtr<UMaterialInterface> MaterialRodape;

    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    float AlturaRodape = 10.f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Visual")
    float EspessuraRodape = 2.f;

    // ================= Botoes =================
    UFUNCTION(CallInEditor, Category = "Gerador")
    void Gerar();

    UFUNCTION(CallInEditor, Category = "Gerador")
    void NovaSemente();

    UFUNCTION(CallInEditor, Category = "Gerador")
    void Limpar();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Gerador|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Paredes;

    UPROPERTY(VisibleAnywhere, Category = "Gerador|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Rodapes;

    UPROPERTY(VisibleAnywhere, Category = "Gerador|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Pilares;

    UPROPERTY(VisibleAnywhere, Category = "Gerador|Componentes")
    TObjectPtr<UStaticMeshComponent> Piso;

    UPROPERTY(VisibleAnywhere, Category = "Gerador|Componentes")
    TObjectPtr<UStaticMeshComponent> Teto;

private:
    UPROPERTY()
    TObjectPtr<UStaticMesh> MalhaCubo;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> Lampadas;
};
