#include "CorridaComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCorridaComponent::UCorridaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCorridaComponent::BeginPlay()
{
	Super::BeginPlay();
	AplicarVelocidade(); // começa andando
}

void UCorridaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCorridaComponent, bCorrendo);
}

void UCorridaComponent::IniciarCorrida() { DefinirCorrida(true); }
void UCorridaComponent::PararCorrida() { DefinirCorrida(false); }

void UCorridaComponent::DefinirCorrida(bool bNovoValor)
{
	if (bCorrendo == bNovoValor) return;

	// Aplica na hora para quem apertou (sem esperar a rede)
	bCorrendo = bNovoValor;
	AplicarVelocidade();

	// Se for um cliente, avisa o servidor
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_DefinirCorrida(bNovoValor);
	}
}

void UCorridaComponent::Server_DefinirCorrida_Implementation(bool bNovoValor)
{
	bCorrendo = bNovoValor;
	AplicarVelocidade();
}

void UCorridaComponent::OnRep_Correndo()
{
	AplicarVelocidade();
}

void UCorridaComponent::AplicarVelocidade()
{
	if (ACharacter* Personagem = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movimento = Personagem->GetCharacterMovement())
		{
			Movimento->MaxWalkSpeed = bCorrendo ? VelocidadeCorrer : VelocidadeAndar;
		}
	}
}