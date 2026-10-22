#include "BalancoCameraComponent.h"
#include "CorridaComponent.h"
#include "PassosComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBalancoCameraComponent::UBalancoCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UBalancoCameraComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Dono = GetOwner())
    {
        Camera = Dono->FindComponentByClass<UCameraComponent>();
        Corrida = Dono->FindComponentByClass<UCorridaComponent>();
        Passos = Dono->FindComponentByClass<UPassosComponent>();
        if (Passos)
        {
            // Calcula os passos antes da camera, no mesmo quadro
            AddTickPrerequisiteComponent(Passos);
        }
    }
}

void UBalancoCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ACharacter* Personagem = Cast<ACharacter>(GetOwner());
    if (!Personagem || !Camera || !Personagem->IsLocallyControlled()) return;

    const UCharacterMovementComponent* Mov = Personagem->GetCharacterMovement();
    const FVector Vel = Personagem->GetVelocity();
    const float Velocidade = Vel.Size2D();
    const bool bNoChao = Mov && Mov->IsMovingOnGround();
    const float Folego = Corrida ? Corrida->GetFolego() : 1.f;

    // Intensidade do balanco segue a velocidade (suave para nao "pular")
    const float Alvo = bNoChao ? FMath::Clamp(Velocidade / 250.f, 0.f, 2.f) : 0.f;
    Intensidade = FMath::FInterpTo(Intensidade, Alvo, DeltaTime, 6.f);

    // Ritmo: o mesmo dos sons de passo (sem eles, calcula pela distancia)
    if (Passos)
    {
        Fase = Passos->GetFase();
    }
    else if (bNoChao)
    {
        Fase += Velocidade * DeltaTime / 150.f;
    }

    const float Forca = FMath::Min(Intensidade, 1.f);
    const float Altura = FMath::Lerp(AlturaAndar, AlturaCorrer, FMath::Clamp(Intensidade - 1.f, 0.f, 1.f));

    FVector Offset = FVector::ZeroVector;
    // Curva suave: mais baixo quando o pe toca o chao, mais alto no meio da passada
    Offset.Z = -FMath::Cos(Fase * 2.f * PI) * Altura * Forca;
    // Um lado por passo (esquerdo, direito)
    Offset.Y = FMath::Sin(Fase * PI) * Lateral * Forca;

    // Respiracao: devagar descansado, rapida e funda quando o folego acaba.
    // A fase ACUMULA (antes era Tempo x Ritmo: quando o ritmo mudava, a camera tremia)
    Cansaco = FMath::FInterpTo(Cansaco, FMath::Clamp(1.f - Folego, 0.f, 1.f), DeltaTime, 2.f);
    const float Ritmo = FMath::Lerp(0.25f, 0.75f, Cansaco);   // respiracoes por segundo
    FaseRespiracao = FMath::Fmod(FaseRespiracao + DeltaTime * Ritmo * 2.f * PI, 2.f * PI);
    Offset.Z += FMath::Sin(FaseRespiracao) * (Respiracao + Ofego * Cansaco * Cansaco);

    // Pouso: afunda e volta
    if (bEstavaNoAr && bNoChao)
    {
        Pouso = AfundarPouso;
    }
    bEstavaNoAr = Mov && Mov->IsFalling();
    Pouso = FMath::FInterpTo(Pouso, 0.f, DeltaTime, 7.f);
    Offset.Z -= Pouso;

    // Inclina um pouco quando anda de lado
    const float Lado = FVector::DotProduct(Vel, Personagem->GetActorRightVector()) / 250.f;
    RollAtual = FMath::FInterpTo(RollAtual, FMath::Clamp(Lado, -1.f, 1.f) * InclinacaoLateral, DeltaTime, 5.f);

    // Aplica so a DIFERENCA, para nao brigar com animacoes ou outros sistemas
    Camera->AddRelativeLocation(Offset - OffsetAnterior);
    FRotator Rot = Camera->GetRelativeRotation();
    Rot.Roll += RollAtual - RollAnterior;
    Camera->SetRelativeRotation(Rot);

    OffsetAnterior = Offset;
    RollAnterior = RollAtual;
}
