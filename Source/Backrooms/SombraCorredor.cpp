#include "SombraCorredor.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

ASombraCorredor::ASombraCorredor()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    Corpo = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Corpo"));
    Corpo->SetupAttachment(RootComponent);
    Corpo->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));   // o boneco olha para +Y; o ator anda para +X
    Corpo->SetRelativeScale3D(FVector(1.05f));                // um pouco alto demais para ser alguem
    Corpo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Corpo->SetCastShadow(false);
    Corpo->bReceivesDecals = false;
    Corpo->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void ASombraCorredor::Iniciar(const FVector& De, const FVector& Para, float NovaDuracao)
{
    Inicio = De;
    Fim = Para;
    Duracao = FMath::Max(0.1f, NovaDuracao);
    Tempo = 0.f;
    bAndando = true;

    SetActorLocationAndRotation(Inicio, (Fim - Inicio).GetSafeNormal2D().Rotation());
    PintarDePreto();
    if (AnimAndando)
    {
        Corpo->SetAnimation(AnimAndando);
        Corpo->SetPlayRate(VelocidadeAnimAndando);
    }
    Corpo->Play(true);
}

void ASombraCorredor::IniciarParado(const FVector& Local, const FVector& OlharPara, float NovaDuracao)
{
    Inicio = Local;
    Fim = Local;
    Duracao = FMath::Max(0.1f, NovaDuracao);
    Tempo = 0.f;
    bParado = true;

    SetActorLocationAndRotation(Local, (OlharPara - Local).GetSafeNormal2D().Rotation());
    PintarDePreto();

    // Parado, quase imovel: so a respiracao da animacao de espera, bem lenta
    if (AnimParado)
    {
        Corpo->SetAnimation(AnimParado);
        Corpo->SetPlayRate(VelocidadeAnimParado);
    }
    Corpo->Play(true);
}

void ASombraCorredor::PintarDePreto()
{
    // Material escolhido no BP_Sombra
    if (UMaterialInterface* Preto = MaterialSombra.Get())
    {
        for (int32 I = 0; I < Corpo->GetNumMaterials(); ++I)
        {
            Corpo->SetMaterial(I, Preto);
        }
    }
}

void ASombraCorredor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bParado)
    {
        Tempo += DeltaSeconds;
        if (Tempo >= Duracao) Destroy();
        return;
    }
    if (!bAndando) return;

    Tempo += DeltaSeconds;
    const float Alpha = FMath::Clamp(Tempo / Duracao, 0.f, 1.f);
    SetActorLocation(FMath::Lerp(Inicio, Fim, Alpha));

    if (Alpha >= 1.f)
    {
        Destroy();
    }
}
