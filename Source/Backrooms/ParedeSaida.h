#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParedeSaida.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UBoxComponent;
class UAudioComponent;
class USoundBase;
class UMaterialInterface;

// A "saida" do Level 0. Fica numa parede comum.
// - Com as luzes acesas: so tem um rabisco "NAO OLHE PARA ELA".
// - No escuro do enigma: aparece uma placa verde de SAIDA, que treme quando vista de canto de olho.
// - Para atravessar: chegar de COSTAS e encostar na parede, sem olhar para ela.
UCLASS()
class BACKROOMS_API AParedeSaida : public AActor
{
    GENERATED_BODY()

public:
    AParedeSaida();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Servidor: mostra/esconde a placa
    void DefinirRevelada(bool bNova);
    bool EstaRevelada() const { return bRevelada; }

    // Som de corrente de ar que aumenta quando voce chega de costas (escolhido no BP_ParedeSaida)
    UPROPERTY(EditAnywhere, Category = "Saida")
    TObjectPtr<USoundBase> SomVento;

    // Quanto tempo encostado de costas para atravessar (s)
    UPROPERTY(EditAnywhere, Category = "Saida")
    float TempoDeCostas = 0.4f;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UStaticMeshComponent> Placa;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UTextRenderComponent> TextoPlaca;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UPointLightComponent> LuzPlaca;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UTextRenderComponent> Aviso;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UBoxComponent> Gatilho;

    UPROPERTY(VisibleAnywhere, Category = "Saida")
    TObjectPtr<UAudioComponent> Vento;

private:
    UPROPERTY(ReplicatedUsing = OnRep_Revelada)
    bool bRevelada = false;

    UFUNCTION()
    void OnRep_Revelada();

    void MostrarPlaca(bool bMostrar);
    void ChecarTravessia(float DeltaSeconds);
    void EfeitoLocal(float DeltaSeconds);

    float TempoEncostado = 0.f;
    float VolumeVento = 0.f;
};
