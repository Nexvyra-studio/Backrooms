#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PassosComponent.generated.h"

class USoundBase;
class USoundAttenuation;

// Som dos passos do personagem: UM passo a cada X cm andados, no ritmo exato do movimento.
// A camera (BalancoCameraComponent) usa a mesma "fase" para balancar junto com o som.
UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UPassosComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPassosComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Passos")
	TArray<TObjectPtr<USoundBase>> SonsPasso;

	UPROPERTY(EditAnywhere, Category = "Passos")
	TObjectPtr<USoundAttenuation> Atenuacao;

	// Distancia (cm) entre um passo e outro andando
	UPROPERTY(EditAnywhere, Category = "Passos", meta = (ClampMin = "10"))
	float DistanciaPasso = 150.f;

	// Distancia (cm) entre passos correndo (a passada fica mais longa)
	UPROPERTY(EditAnywhere, Category = "Passos", meta = (ClampMin = "10"))
	float DistanciaPassoCorrendo = 195.f;

	// Velocidade (cm/s) considerada "correndo" para a passada longa
	UPROPERTY(EditAnywhere, Category = "Passos")
	float VelocidadeCorrida = 500.f;

	// Abaixo dessa velocidade (cm/s) nao conta como andando
	UPROPERTY(EditAnywhere, Category = "Passos")
	float VelocidadeMinima = 60.f;

	UPROPERTY(EditAnywhere, Category = "Passos")
	float Volume = 0.42f;

	// Correndo o passo e mais forte
	UPROPERTY(EditAnywhere, Category = "Passos")
	float VolumeCorrendo = 1.35f;

	UPROPERTY(EditAnywhere, Category = "Passos")
	FVector2D VariacaoPitch = FVector2D(0.94f, 1.06f);

	UPROPERTY(EditAnywhere, Category = "Passos")
	FVector2D VariacaoVolume = FVector2D(0.85f, 1.0f);

	// Fase continua dos passos: a parte inteira conta os passos, a fracao vai de 0 (pe no chao) ate 1 (proximo passo)
	float GetFase() const { return Fase; }

protected:
	virtual void BeginPlay() override;

private:
	void TocarPasso(float MultiplicadorVolume);

	FVector UltimaPosicao = FVector::ZeroVector;
	float Fase = 0.f;
	int32 UltimoIndice = INDEX_NONE;
	bool bEstavaNoAr = false;
};
