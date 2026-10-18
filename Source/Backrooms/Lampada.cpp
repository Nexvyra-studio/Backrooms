#include "Lampada.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

ALampada::ALampada()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    // Placa
    Placa = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placa"));
    Placa->SetupAttachment(RootComponent);
    Placa->SetRelativeScale3D(FVector(1.2f, 0.6f, 0.02f));
    Placa->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cubo.Succeeded())
    {
        Placa->SetStaticMesh(Cubo.Object);
    }

    // Luz
    Luz = CreateDefaultSubobject<URectLightComponent>(TEXT("Luz"));
    Luz->SetupAttachment(RootComponent);
    Luz->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
    Luz->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    Luz->IntensityUnits = ELightUnits::Candelas;
    Luz->Intensity = IntensidadeAcesa;
    Luz->LightColor = FColor(255, 231, 174);
    Luz->SourceWidth = 120.f;
    Luz->SourceHeight = 30.f;
    Luz->AttenuationRadius = 500.f;

    // Zumbido (não toca sozinho; quem dá o play é o BeginPlay)
    Zumbido = CreateDefaultSubobject<UAudioComponent>(TEXT("Zumbido"));
    Zumbido->SetupAttachment(RootComponent);
    Zumbido->bAutoActivate = false;
}

void ALampada::BeginPlay()
{
    Super::BeginPlay();

    // Liga o zumbido
    if (SomZumbido)
    {
        Zumbido->SetSound(SomZumbido);
        if (Atenuacao)
        {
            Zumbido->AttenuationSettings = Atenuacao;
        }
        Zumbido->SetVolumeMultiplier(VolumeZumbido);

        // Começa num ponto aleatório do áudio, para várias lâmpadas não zumbirem em sincronia
        const float Duracao = SomZumbido->GetDuration();
        Zumbido->Play(Duracao > 0.f ? FMath::FRandRange(0.f, Duracao) : 0.f);
    }

    DefinirAcesa(true);

    GetWorldTimerManager().SetTimer(TimerChecagem, this, &ALampada::Checar, IntervaloChecagem, true);
}

void ALampada::Checar()
{
    if (PiscadasRestantes > 0) return;
    if (FMath::FRand() >= ChanceDeRajada) return;

    PiscadasRestantes = FMath::RandRange(MinPiscadas, FMath::Max(MinPiscadas, MaxPiscadas));
    Apagar();
}

void ALampada::Apagar()
{
    TocarEstalo(SonsApagar);   // "tic"
    DefinirAcesa(false);

    const float Tempo = FMath::FRandRange(DuracaoApagada.X, DuracaoApagada.Y);
    GetWorldTimerManager().SetTimer(TimerRajada, this, &ALampada::Acender, Tempo, false);
}

void ALampada::Acender()
{
    TocarEstalo(SonsAcender);  // "tac"
    DefinirAcesa(true);
    --PiscadasRestantes;

    if (PiscadasRestantes > 0)
    {
        const float Tempo = FMath::FRandRange(DuracaoAcesaEntre.X, DuracaoAcesaEntre.Y);
        GetWorldTimerManager().SetTimer(TimerRajada, this, &ALampada::Apagar, Tempo, false);
    }
}

void ALampada::DefinirAcesa(bool bAcesa)
{
    Luz->SetIntensity(bAcesa ? IntensidadeAcesa : 0.f);

    if (UMaterialInterface* Material = bAcesa ? MaterialAceso : MaterialApagado)
    {
        Placa->SetMaterial(0, Material);
    }

    // O zumbido "corta" quando a luz apaga (volume quase zero, sem parar o som)
    if (Zumbido && Zumbido->IsPlaying())
    {
        Zumbido->SetVolumeMultiplier(bAcesa ? VolumeZumbido : 0.001f);
    }
}

void ALampada::TocarEstalo(const TArray<TObjectPtr<USoundBase>>& Lista)
{
    if (Lista.Num() == 0) return;

    USoundBase* Som = Lista[FMath::RandRange(0, Lista.Num() - 1)];
    if (!Som) return;

    const float Volume = VolumeEstalo * FMath::FRandRange(VariacaoVolume.X, VariacaoVolume.Y);
    const float Pitch = FMath::FRandRange(VariacaoPitch.X, VariacaoPitch.Y);

    UGameplayStatics::PlaySoundAtLocation(this, Som, Placa->GetComponentLocation(), Volume, Pitch, 0.f, Atenuacao);
}