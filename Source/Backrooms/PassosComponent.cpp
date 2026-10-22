#include "PassosComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UPassosComponent::UPassosComponent()
{
	// Roda todo quadro: o passo cai exatamente quando o pe "toca" o chao (antes era a cada 0,05 s)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UPassosComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Dono = GetOwner())
	{
		UltimaPosicao = Dono->GetActorLocation();
	}
	// Comeca no meio de uma passada: o primeiro passo vem logo que comecar a andar
	Fase = 0.55f;
}

void UPassosComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Personagem = Cast<ACharacter>(GetOwner());
	if (!Personagem) return;

	const FVector Posicao = Personagem->GetActorLocation();
	FVector Delta = Posicao - UltimaPosicao;
	Delta.Z = 0.f;
	UltimaPosicao = Posicao;

	const UCharacterMovementComponent* Movimento = Personagem->GetCharacterMovement();
	const bool bNoChao = Movimento && Movimento->IsMovingOnGround();
	const float Velocidade = Personagem->GetVelocity().Size2D();

	// Pousou de um pulo/queda: um passo mais forte na hora
	if (bEstavaNoAr && bNoChao)
	{
		TocarPasso(1.3f);
		Fase = FMath::FloorToFloat(Fase) + 1.f;
	}
	bEstavaNoAr = Movimento && Movimento->IsFalling();

	if (!bNoChao || Velocidade < VelocidadeMinima) return;

	// Teleporte / correcao grande da rede: ignora
	const float Andou = Delta.Size();
	if (Andou > 200.f) return;

	// Passada mais longa quanto mais rapido
	const float Correndo = FMath::Clamp((Velocidade - 250.f) / FMath::Max(1.f, VelocidadeCorrida - 250.f), 0.f, 1.f);
	const float Passada = FMath::Lerp(DistanciaPasso, DistanciaPassoCorrendo, Correndo);

	const float Antes = Fase;
	Fase += Andou / Passada;

	// Cruzou um numero inteiro = pe no chao
	if (FMath::FloorToFloat(Fase) > FMath::FloorToFloat(Antes))
	{
		TocarPasso(FMath::Lerp(1.f, VolumeCorrendo, Correndo));
	}

	// Mantem o numero pequeno (precisao) sem perder a paridade esquerda/direita
	if (Fase > 1000.f) Fase -= 1000.f;
}

void UPassosComponent::TocarPasso(float MultiplicadorVolume)
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
	if (const UCapsuleComponent* Capsula = Personagem->GetCapsuleComponent())
	{
		Pe.Z -= Capsula->GetScaledCapsuleHalfHeight();
	}

	UGameplayStatics::PlaySoundAtLocation(
		this, Som, Pe,
		Volume * MultiplicadorVolume * FMath::FRandRange(VariacaoVolume.X, VariacaoVolume.Y),
		FMath::FRandRange(VariacaoPitch.X, VariacaoPitch.Y),
		0.f, Atenuacao);
}
