#include "PassosComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"

UPassosComponent::UPassosComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPassosComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Dono = GetOwner())
	{
		UltimaPosicao = Dono->GetActorLocation();
	}

	GetWorld()->GetTimerManager().SetTimer(
		TimerChecagem, this, &UPassosComponent::Checar, IntervaloChecagem, true);
}

void UPassosComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TimerChecagem);
	Super::EndPlay(EndPlayReason);
}

void UPassosComponent::Checar()
{
	ACharacter* Personagem = Cast<ACharacter>(GetOwner());
	if (!Personagem) return;

	const FVector Posicao = Personagem->GetActorLocation();
	UCharacterMovementComponent* Movimento = Personagem->GetCharacterMovement();
	const float Velocidade = Personagem->GetVelocity().Size2D();

	if (Movimento && Movimento->IsMovingOnGround() && Velocidade < VelocidadeMinima)
	{
		// Parado ou quase parado: zera para o proximo passo comecar do zero
		DistanciaAcumulada = 0.f;

		// Corta o passo que ainda esta tocando
		if (SomAtual && SomAtual->IsPlaying())
		{
			SomAtual->FadeOut(FadeAoParar, 0.f);
		}
		SomAtual = nullptr;
	}
	else if (Movimento && Movimento->IsMovingOnGround())
	{
		FVector Delta = Posicao - UltimaPosicao;
		Delta.Z = 0.f; // ignora subida/descida

		DistanciaAcumulada += Delta.Size();

		if (DistanciaAcumulada >= DistanciaPasso)
		{
			DistanciaAcumulada = 0.f;
			TocarPasso();
		}
	}
	else
	{
		// No ar: ao pousar, o primeiro passo vem mais rapido
		DistanciaAcumulada = DistanciaPasso * 0.5f;
	}

	UltimaPosicao = Posicao;
}

void UPassosComponent::TocarPasso()
{
	if (SonsPasso.Num() == 0) return;

	// Sorteia um som, evitando repetir o anterior
	int32 Indice = FMath::RandRange(0, SonsPasso.Num() - 1);
	if (SonsPasso.Num() > 1 && Indice == UltimoIndice)
	{
		Indice = (Indice + 1) % SonsPasso.Num();
	}
	UltimoIndice = Indice;

	USoundBase* Som = SonsPasso[Indice];
	ACharacter* Personagem = Cast<ACharacter>(GetOwner());
	if (!Som || !Personagem) return;

	// Toca na altura dos pes
	FVector Pe = Personagem->GetActorLocation();
	if (UCapsuleComponent* Capsula = Personagem->GetCapsuleComponent())
	{
		Pe.Z -= Capsula->GetScaledCapsuleHalfHeight();
	}

	SomAtual = UGameplayStatics::SpawnSoundAtLocation(
		this, Som, Pe, FRotator::ZeroRotator,
		Volume * FMath::FRandRange(VariacaoVolume.X, VariacaoVolume.Y),
		FMath::FRandRange(VariacaoPitch.X, VariacaoPitch.Y),
		0.f, Atenuacao);
}