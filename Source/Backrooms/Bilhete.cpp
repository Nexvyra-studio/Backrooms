#include "Bilhete.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABilhete::ABilhete()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    // Folha A4 (21 x 29,7 cm) levemente torta, a 1,45 m do chao
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Papel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Papel"));
    Papel->SetupAttachment(RootComponent);
    if (Cubo.Succeeded()) Papel->SetStaticMesh(Cubo.Object);
    Papel->SetRelativeLocation(FVector(0.2f, 0.f, 145.f));
    Papel->SetRelativeRotation(FRotator(0.f, 0.f, 3.f));
    Papel->SetRelativeScale3D(FVector(0.003f, 0.21f, 0.297f));
    Papel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Papel->SetCastShadow(false);

    Escrita = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Escrita"));
    Escrita->SetupAttachment(RootComponent);
    Escrita->SetRelativeLocation(FVector(0.6f, 0.f, 145.f));
    Escrita->SetRelativeRotation(FRotator(0.f, 0.f, 3.f));
    Escrita->SetHorizontalAlignment(EHTA_Center);
    Escrita->SetVerticalAlignment(EVRTA_TextCenter);
    Escrita->SetWorldSize(1.8f);
    Escrita->SetTextRenderColor(FColor(20, 25, 60));   // caneta azul

    // O material do papel e escolhido no Blueprint (componente "Papel" do BP_Bilhete)
}

void ABilhete::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    Escrita->SetText(Texto);
}

void ABilhete::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bMudou || TextoDepois.IsEmpty())
    {
        SetActorTickEnabled(false);
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    FVector Olho;
    FRotator Rot;
    PC->GetPlayerViewPoint(Olho, Rot);

    const FVector PosPapel = Escrita->GetComponentLocation();
    const float Distancia = FVector::Dist(Olho, PosPapel);
    const float Olhando = FVector::DotProduct(Rot.Vector(), (PosPapel - Olho).GetSafeNormal());

    if (!bLido)
    {
        // Leu de verdade: perto e olhando para o papel por um tempinho
        TempoLendo = (Distancia < 220.f && Olhando > 0.85f) ? TempoLendo + DeltaSeconds : 0.f;
        bLido = TempoLendo >= 1.5f;
        return;
    }

    // Foi embora e nao esta olhando: o texto muda sem ninguem ver
    if (Distancia > 900.f && Olhando < 0.3f)
    {
        Escrita->SetText(TextoDepois);
        bMudou = true;
    }
}
