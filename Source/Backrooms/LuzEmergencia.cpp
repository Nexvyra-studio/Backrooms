#include "LuzEmergencia.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALuzEmergencia::ALuzEmergencia()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));

    // Seta em forma de ">" apontando para +X (a direcao da saida).
    // O material verde e escolhido no BP_LuzEmergencia (componentes BracoEsquerdo e BracoDireito).
    auto CriarBraco = [&](const TCHAR* Nome, float Yaw, float Y)
    {
        UStaticMeshComponent* B = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
        B->SetupAttachment(RootComponent);
        if (Cubo.Succeeded())  B->SetStaticMesh(Cubo.Object);
        B->SetRelativeLocation(FVector(0.f, Y, 0.f));
        B->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
        B->SetRelativeScale3D(FVector(0.32f, 0.05f, 0.03f));
        B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        B->SetCastShadow(false);
        return B;
    };
    BracoEsquerdo = CriarBraco(TEXT("BracoEsquerdo"), 35.f, -9.f);
    BracoDireito = CriarBraco(TEXT("BracoDireito"), -35.f, 9.f);

    Luz = CreateDefaultSubobject<UPointLightComponent>(TEXT("Luz"));
    Luz->SetupAttachment(RootComponent);
    Luz->SetRelativeLocation(FVector(0.f, 0.f, -12.f));
    Luz->SetIntensityUnits(ELightUnits::Candelas);
    Luz->SetIntensity(1.2f);
    Luz->SetLightColor(FLinearColor(0.12f, 1.f, 0.3f));
    Luz->SetAttenuationRadius(320.f);
    Luz->SetCastShadows(false);
}

void ALuzEmergencia::BeginPlay()
{
    Super::BeginPlay();
    Fase = FMath::FRandRange(0.f, 6.f);
}

void ALuzEmergencia::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Bateria fraca: a luz "respira" e falha de vez em quando
    Tempo += DeltaSeconds;
    float Brilho = 0.75f + 0.25f * FMath::Sin((Tempo + Fase) * 2.3f);
    if (FMath::FRand() < 0.015f) Brilho = 0.1f;
    Luz->SetIntensity(1.2f * Brilho);
}
