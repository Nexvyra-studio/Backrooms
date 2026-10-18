#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeradorLabirinto.generated.h"
class UStaticMesh;
class UMaterialInterface;

class ACelula;

// Gera um labirinto de células a partir de uma semente (seed).
// Mesma semente = mesmo labirinto (importante para o multiplayer).
UCLASS()
class BACKROOMS_API AGeradorLabirinto : public AActor
{
    GENERATED_BODY()

public:
    AGeradorLabirinto();

    // Qual célula usar (coloque o BP_Celula aqui)
    UPROPERTY(EditAnywhere, Category = "Gerador")
    TSubclassOf<ACelula> ClasseCelula;

    // Tamanho do labirinto em células (eixo X e eixo Y)
    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "1", ClampMax = "50"))
    int32 Colunas = 6;

    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "1", ClampMax = "50"))
    int32 Linhas = 6;

    // O "código" do labirinto: mesmo número = mesmo labirinto
    UPROPERTY(EditAnywhere, Category = "Gerador")
    int32 Semente = 1234;

    // Chance de abrir paredes extras (cria atalhos, voltas e áreas abertas)
    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceAberturaExtra = 0.15f;

    // Chance de cada célula ter lâmpada (menos lâmpadas = mais escuro e mais leve)
    UPROPERTY(EditAnywhere, Category = "Gerador", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampada = 0.7f;

    // ---------- Salões abertos ----------
// Quantos salões abrir no labirinto
    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "0"))
    int32 NumeroDeSaloes = 3;

    // Tamanho dos salões, em células (mínimo e máximo)
    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "2"))
    int32 TamanhoSalaoMin = 2;

    UPROPERTY(EditAnywhere, Category = "Gerador|Saloes", meta = (ClampMin = "2"))
    int32 TamanhoSalaoMax = 4;

    // ---------- Pilares ----------
    // Chance de surgir um pilar em cada canto aberto
    UPROPERTY(EditAnywhere, Category = "Gerador|Pilares", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePilar = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Gerador|Pilares")
    TObjectPtr<UStaticMesh> MalhaPilar;

    UPROPERTY(EditAnywhere, Category = "Gerador|Pilares")
    TObjectPtr<UMaterialInterface> MaterialPilar;

    // ---------- Botões que aparecem no painel Detalhes ----------
    UFUNCTION(CallInEditor, Category = "Gerador")
    void Gerar();

    UFUNCTION(CallInEditor, Category = "Gerador")
    void NovaSemente();

    UFUNCTION(CallInEditor, Category = "Gerador")
    void Limpar();

private:
    // Guarda as células criadas, para poder apagar depois
    UPROPERTY()
    TArray<TObjectPtr<ACelula>> Celulas;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> Pilares;
};