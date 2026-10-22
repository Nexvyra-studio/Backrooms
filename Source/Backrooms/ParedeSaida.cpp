#include "ParedeSaida.h"
#include "EnigmaLevel0.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// O ator fica no chao, encostado na parede, com +X apontando para fora dela (para o corredor).
AParedeSaida::AParedeSaida()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;
    bReplicates = true;
    bAlwaysRelevant = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));

    // Placa verde de SAIDA, no alto, como em cima de uma porta
    Placa = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placa"));
    Placa->SetupAttachment(RootComponent);
    if (Cubo.Succeeded()) Placa->SetStaticMesh(Cubo.Object);
    Placa->SetRelativeLocation(FVector(4.f, 0.f, 228.f));
    Placa->SetRelativeScale3D(FVector(0.08f, 0.36f, 0.14f));
    Placa->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Placa->SetCastShadow(false);

    TextoPlaca = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextoPlaca"));
    TextoPlaca->SetupAttachment(RootComponent);
    TextoPlaca->SetRelativeLocation(FVector(8.2f, 0.f, 228.f));
    TextoPlaca->SetHorizontalAlignment(EHTA_Center);
    TextoPlaca->SetVerticalAlignment(EVRTA_TextCenter);
    TextoPlaca->SetWorldSize(9.f);
    TextoPlaca->SetTextRenderColor(FColor(235, 255, 235));
    TextoPlaca->SetText(FText::FromString(TEXT("SA\u00CDDA")));

    LuzPlaca = CreateDefaultSubobject<UPointLightComponent>(TEXT("LuzPlaca"));
    LuzPlaca->SetupAttachment(RootComponent);
    LuzPlaca->SetRelativeLocation(FVector(25.f, 0.f, 228.f));
    LuzPlaca->SetIntensityUnits(ELightUnits::Candelas);
    LuzPlaca->SetIntensity(4.f);
    LuzPlaca->SetLightColor(FLinearColor(0.15f, 1.f, 0.35f));
    LuzPlaca->SetAttenuationRadius(450.f);
    LuzPlaca->SetCastShadows(false);

    // Rabisco sempre visivel
    Aviso = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Aviso"));
    Aviso->SetupAttachment(RootComponent);
    Aviso->SetRelativeLocation(FVector(0.6f, 0.f, 140.f));
    Aviso->SetHorizontalAlignment(EHTA_Center);
    Aviso->SetVerticalAlignment(EVRTA_TextCenter);
    Aviso->SetWorldSize(13.f);
    Aviso->SetTextRenderColor(FColor(60, 12, 8));
    Aviso->SetText(FText::FromString(TEXT("N\u00C3O OLHE\nPARA ELA.")));

    // Area na frente da parede onde a travessia acontece
    Gatilho = CreateDefaultSubobject<UBoxComponent>(TEXT("Gatilho"));
    Gatilho->SetupAttachment(RootComponent);
    Gatilho->SetRelativeLocation(FVector(55.f, 0.f, 100.f));
    Gatilho->SetBoxExtent(FVector(55.f, 85.f, 100.f));
    Gatilho->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Gatilho->SetGenerateOverlapEvents(true);

    Vento = CreateDefaultSubobject<UAudioComponent>(TEXT("Vento"));
    Vento->SetupAttachment(RootComponent);
    Vento->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
    Vento->bAutoActivate = false;

    // O material verde da placa e escolhido no BP_ParedeSaida (componente "Placa")
}

void AParedeSaida::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // No editor a placa aparece, para voce achar onde ela esta
    MostrarPlaca(!GetWorld() || !GetWorld()->IsGameWorld());
}

void AParedeSaida::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AParedeSaida, bRevelada);
}

void AParedeSaida::BeginPlay()
{
    Super::BeginPlay();
    MostrarPlaca(bRevelada);

    if (SomVento)
    {
        Vento->SetSound(SomVento);
        Vento->SetVolumeMultiplier(0.001f);
        Vento->Play();
    }
}

void AParedeSaida::DefinirRevelada(bool bNova)
{
    if (!HasAuthority()) return;
    bRevelada = bNova;
    TempoEncostado = 0.f;
    MostrarPlaca(bRevelada);
}

void AParedeSaida::OnRep_Revelada()
{
    MostrarPlaca(bRevelada);
}

void AParedeSaida::MostrarPlaca(bool bMostrar)
{
    Placa->SetVisibility(bMostrar);
    TextoPlaca->SetVisibility(bMostrar);
    LuzPlaca->SetVisibility(bMostrar);
}

void AParedeSaida::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bRevelada)
    {
        VolumeVento = 0.f;
        if (Vento->IsPlaying()) Vento->SetVolumeMultiplier(0.001f);
        return;
    }

    EfeitoLocal(DeltaSeconds);
    if (HasAuthority())
    {
        ChecarTravessia(DeltaSeconds);
    }
}

void AParedeSaida::EfeitoLocal(float DeltaSeconds)
{
    // So para o jogador desta maquina
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    APawn* Eu = PC ? PC->GetPawn() : nullptr;
    if (!Eu) return;

    FVector Olho;
    FRotator Rot;
    PC->GetPlayerViewPoint(Olho, Rot);

    const FVector ParaPlaca = (Placa->GetComponentLocation() - Olho);
    const float Distancia = ParaPlaca.Size();
    const float Alinhamento = FVector::DotProduct(Rot.Vector(), ParaPlaca.GetSafeNormal());

    // Olhando direto: a placa fica firme. De canto de olho: ela treme.
    const bool bCantoDeOlho = Distancia < 900.f && Alinhamento < 0.75f;
    if (bCantoDeOlho)
    {
        MostrarPlaca(FMath::FRand() > 0.18f);
    }
    else
    {
        MostrarPlaca(true);
    }

    // Vento: mais forte quanto mais perto e mais "de costas" voce estiver
    const FVector Normal = GetActorForwardVector();
    const float DeCostas = FMath::Clamp(FVector::DotProduct(Rot.Vector().GetSafeNormal2D(), Normal), 0.f, 1.f);
    const float Perto = FMath::Clamp(1.f - (Distancia - 100.f) / 500.f, 0.f, 1.f);
    VolumeVento = FMath::FInterpTo(VolumeVento, Perto * (0.25f + 0.75f * DeCostas), DeltaSeconds, 2.f);
    if (Vento->IsPlaying())
    {
        Vento->SetVolumeMultiplier(FMath::Max(0.001f, VolumeVento));
    }
}

void AParedeSaida::ChecarTravessia(float DeltaSeconds)
{
    const FVector Normal = GetActorForwardVector();   // aponta para fora da parede
    bool bAlguemDeCostas = false;
    APawn* Quem = nullptr;

    TArray<AActor*> Dentro;
    Gatilho->GetOverlappingActors(Dentro, APawn::StaticClass());
    for (AActor* A : Dentro)
    {
        APawn* P = Cast<APawn>(A);
        if (!P || !P->IsPlayerControlled()) continue;

        // Olhando para o lado oposto da parede...
        const FVector Visao = P->GetBaseAimRotation().Vector().GetSafeNormal2D();
        const bool bDeCostas = FVector::DotProduct(Visao, Normal) > 0.6f;

        // ...e encostado nela
        const float DistParede = FVector::DotProduct(P->GetActorLocation() - GetActorLocation(), Normal);
        if (bDeCostas && DistParede < 75.f)
        {
            bAlguemDeCostas = true;
            Quem = P;
            break;
        }
    }

    TempoEncostado = bAlguemDeCostas ? TempoEncostado + DeltaSeconds : 0.f;
    if (TempoEncostado >= TempoDeCostas)
    {
        TempoEncostado = -1000.f;   // nao dispara de novo
        for (TActorIterator<AEnigmaLevel0> It(GetWorld()); It; ++It)
        {
            It->Concluir(Quem);
            break;
        }
    }
}
