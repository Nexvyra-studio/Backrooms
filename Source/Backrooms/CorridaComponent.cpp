#include "CorridaComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCorridaComponent::UCorridaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
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

void UCorridaComponent::IniciarCorrida()
{
	bQuerCorrer = true;
	if (!bExausto) DefinirCorrida(true);
}

void UCorridaComponent::PararCorrida()
{
	bQuerCorrer = false;
	DefinirCorrida(false);
}

void UCorridaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// O folego e controlado por quem joga (o servidor so recebe "correndo/andando")
	const APawn* Dono = Cast<APawn>(GetOwner());
	if (!Dono || !Dono->IsLocallyControlled()) return;

	const bool bMexendo = Dono->GetVelocity().Size2D() > 120.f;
	if (bCorrendo && bMexendo)
	{
		Folego = FMath::Max(0.f, Folego - DeltaTime / DuracaoFolego);
		if (Folego <= 0.f)
		{
			// Sem ar: obrigado a andar
			bExausto = true;
			DefinirCorrida(false);
		}
	}
	else
	{
		Folego = FMath::Min(1.f, Folego + DeltaTime / TempoRecuperar);
		if (bExausto && Folego >= FolegoParaVoltar)
		{
			bExausto = false;
			// Ainda segurando Shift? Volta a correr sozinho
			if (bQuerCorrer) DefinirCorrida(true);
		}
	}
}

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