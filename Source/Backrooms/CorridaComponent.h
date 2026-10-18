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
	void AplicarVelocidade();
};