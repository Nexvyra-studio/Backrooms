#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lampada.generated.h"

class UStaticMeshComponent;
class URectLightComponent;
class UMaterialInterface;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;

UCLASS()
class BACKROOMS_API ALampada : public AActor
{
    GENERATED_BODY()

public:
    ALampada();

protected:
    virtual void BeginPlay() override;

    // ---------- Componentes ----------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lampada")
    TObjectPtr<UStaticMeshComponent> Placa;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lampada")
    TObjectPtr<URectLightComponent> Luz;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lampada")
    TObjectPtr<UAudioComponent> Zumbido;

    // ---------- Materiais ----------
    UPROPERTY(EditAnywhere, Category = "Lampada|Materiais")
    TObjectPtr<UMaterialInterface> MaterialAceso;

    UPROPERTY(EditAnywhere, Category = "Lampada|Materiais")
    TObjectPtr<UMaterialInterface> MaterialApagado;

    // ---------- Luz ----------
    UPROPERTY(EditAnywhere, Category = "Lampada|Luz")
    float IntensidadeAcesa = 20.f;

    // ---------- Piscar ----------
    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar", meta = (ClampMin = "0.01"))
    float IntervaloChecagem = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceDeRajada = 0.02f;

    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar", meta = (ClampMin = "1"))
    int32 MinPiscadas = 1;

    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar", meta = (ClampMin = "1"))
    int32 MaxPiscadas = 2;

    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar")
    FVector2D DuracaoApagada = FVector2D(0.03f, 0.08f);

    UPROPERTY(EditAnywhere, Category = "Lampada|Piscar")
    FVector2D DuracaoAcesaEntre = FVector2D(0.08f, 0.2f);

    // ---------- Som ----------
    // Zumbido contínuo (lembre de marcar Looping no Sound Wave)
    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    TObjectPtr<USoundBase> SomZumbido;

    // Um destes toca (sorteado) quando a luz APAGA
    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    TArray<TObjectPtr<USoundBase>> SonsApagar;

    // Um destes toca (sorteado) quando a luz ACENDE
    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    TArray<TObjectPtr<USoundBase>> SonsAcender;

    // Som 3D: como o volume cai com a distância
    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    TObjectPtr<USoundAttenuation> Atenuacao;

    UPROPERTY(EditAnywhere, Category = "Lampada|Som", meta = (ClampMin = "0.0"))
    float VolumeZumbido = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Lampada|Som", meta = (ClampMin = "0.0"))
    float VolumeEstalo = 1.0f;

    // Cada estalo sai com tom e volume levemente diferentes (mín. X, máx. Y)
    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    FVector2D VariacaoPitch = FVector2D(0.9f, 1.1f);

    UPROPERTY(EditAnywhere, Category = "Lampada|Som")
    FVector2D VariacaoVolume = FVector2D(0.8f, 1.0f);

private:
    void Checar();
    void Apagar();
    void Acender();
    void DefinirAcesa(bool bAcesa);
    void TocarEstalo(const TArray<TObjectPtr<USoundBase>>& Lista);

    FTimerHandle TimerChecagem;
    FTimerHandle TimerRajada;
    int32 PiscadasRestantes = 0;
};