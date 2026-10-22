#include "AmbienteLevel0.h"
#include "Components/AudioComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AAmbienteLevel0::AAmbienteLevel0()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    Sala = CreateDefaultSubobject<UAudioComponent>(TEXT("Sala"));
    Sala->SetupAttachment(RootComponent);
    Sala->bAutoActivate = false;
    Sala->bIsUISound = false;
    Sala->bAllowSpatialization = false;   // som "de todo lugar"

}

void AAmbienteLevel0::BeginPlay()
{
    Super::BeginPlay();

    if (USoundBase* Som = SomSala.Get())
    {
        Sala->SetSound(Som);
        Sala->SetVolumeMultiplier(VolumeSala);
        // Comeca do silencio junto com a imagem
        Sala->FadeIn(bIntro ? TempoPreto + TempoClarear : 1.f, 1.f, FMath::FRandRange(0.f, 10.f));
    }

    if (bIntro)
    {
        // Espera o controle do jogador existir
        FTimerHandle H;
        GetWorldTimerManager().SetTimer(H, this, &AAmbienteLevel0::ComecarIntro, 0.05f, false);
    }
}

void AAmbienteLevel0::ComecarIntro()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager) return;

    // Preto total agora...
    PC->PlayerCameraManager->StartCameraFade(1.f, 1.f, 0.01f, FLinearColor::Black, true, true);

    if (USoundBase* Som = SomChegada.Get())
    {
        UGameplayStatics::PlaySound2D(this, Som, 1.f);
    }

    // ...e a imagem volta devagar, como quem acorda
    TWeakObjectPtr<APlayerController> Fraco(PC);
    const float Clarear = TempoClarear;
    FTimerHandle H;
    GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [Fraco, Clarear]()
    {
        if (APlayerController* P = Fraco.Get())
        {
            if (P->PlayerCameraManager)
            {
                P->PlayerCameraManager->StartCameraFade(1.f, 0.f, Clarear, FLinearColor::Black, true, false);
            }
        }
    }), FMath::Max(0.1f, TempoPreto), false);
}

void AAmbienteLevel0::Abafar(float Alvo, float Tempo)
{
    if (Sala && Sala->IsPlaying())
    {
        Sala->AdjustVolume(FMath::Max(0.01f, Tempo), FMath::Clamp(Alvo, 0.001f, 1.f));
    }
}
