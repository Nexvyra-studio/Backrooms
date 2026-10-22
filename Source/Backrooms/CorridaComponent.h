#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CorridaComponent.generated.h"

UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UCorridaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCorridaComponent();

	UPROPERTY(EditAnywhere, Category = "Corrida")
	float VelocidadeAndar = 250.f;

	UPROPERTY(EditAnywhere, Category = "Corrida")
	float VelocidadeCorrer = 500.f;

	UFUNCTION(BlueprintCallable, Category = "Corrida")
	void IniciarCorrida();

	UFUNCTION(BlueprintCallable, Category = "Corrida")
	void PararCorrida();

	UFUNCTION(BlueprintPure, Category = "Corrida")
	bool EstaCorrendo() const { return bCorrendo; }

	// ---------- Folego ----------
	// Segundos de corrida com o folego cheio
	UPROPERTY(EditAnywhere, Category = "Corrida|Folego", meta = (ClampMin = "1.0"))
	float DuracaoFolego = 7.f;

	// Segundos para recuperar o folego inteiro (andando ou parado)
	UPROPERTY(EditAnywhere, Category = "Corrida|Folego", meta = (ClampMin = "1.0"))
	float TempoRecuperar = 9.f;

	// Depois de esgotar, so volta a correr quando recuperar esta fracao
	UPROPERTY(EditAnywhere, Category = "Corrida|Folego", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FolegoParaVoltar = 0.35f;

	// 1 = cheio, 0 = sem ar (usado pelo balanco da camera para "ofegar")
	UFUNCTION(BlueprintPure, Category = "Corrida|Folego")
	float GetFolego() const { return Folego; }

	UFUNCTION(BlueprintPure, Category = "Corrida|Folego")
	bool EstaExausto() const { return bExausto; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Correndo)
	bool bCorrendo = false;

	UFUNCTION()
	void OnRep_Correndo();

	UFUNCTION(Server, Reliable)
	void Server_DefinirCorrida(bool bNovoValor);

	void DefinirCorrida(bool bNovoValor);

	float Folego = 1.f;
	bool bExausto = false;
	bool bQuerCorrer = false;
	void AplicarVelocidade();
};