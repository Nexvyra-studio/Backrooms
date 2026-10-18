#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PassosComponent.generated.h"
class UAudioComponent;

class USoundBase;
class USoundAttenuation;

UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UPassosComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPassosComponent();

	UPROPERTY(EditAnywhere, Category = "Passos")
	TArray<TObjectPtr<USoundBase>> SonsPasso;

	UPROPERTY(EditAnywhere, Category = "Passos")
	TObjectPtr<USoundAttenuation> Atenuacao;

	// Distancia percorrida (cm) entre um passo e outro
	UPROPERTY(EditAnywhere, Category = "Passos", meta = (ClampMin = "10"))
	float DistanciaPasso = 150.f;

	// Abaixo dessa velocidade (cm/s) nao toca passos
	UPROPERTY(EditAnywhere, Category = "Passos")
	float VelocidadeMinima = 80.f;

	UPROPERTY(EditAnywhere, Category = "Passos")
	float Volume = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Passos")
	FVector2D VariacaoPitch = FVector2D(0.92f, 1.08f);

	UPROPERTY(EditAnywhere, Category = "Passos")
	FVector2D VariacaoVolume = FVector2D(0.8f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Passos")
	float IntervaloChecagem = 0.05f;

	// Tempo (s) do fade que corta o passo quando o personagem para
	UPROPERTY(EditAnywhere, Category = "Passos")
	float FadeAoParar = 0.08f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Checar();
	void TocarPasso();

	FTimerHandle TimerChecagem;
	FVector UltimaPosicao = FVector::ZeroVector;
	float DistanciaAcumulada = 0.f;
	int32 UltimoIndice = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UAudioComponent> SomAtual;
};