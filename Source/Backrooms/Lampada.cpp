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

void ALampada::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Mostra no editor como a lampada vai ficar no jogo
    DefinirAcesa(!bQueimada);
}

void ALampada::BeginPlay()
{
    Super::BeginPlay();

    // Queimada: fica apagada, sem zumbido e sem piscar
    if (bQueimada)
    {
        DefinirAcesa(false);
        return;
    }

    // Liga o zumbido
    if (SomZumbido)
    {
        Zumbido->SetSound(SomZumbido);
        if (Atenuacao)
        {
            Zumbido->AttenuationSettings = Atenuacao;
        }
        Zumbido->SetVolumeMultiplier(bSemZumbido ? 0.001f : VolumeZumbido);

        // Começa num ponto aleatório do áudio, para várias lâmpadas não zumbirem em sincronia
        const float Duracao = SomZumbido->GetDuration();
        Zumbido->Play(Duracao > 0.f ? FMath::FRandRange(0.f, Duracao) : 0.f);
    }

    DefinirAcesa(true);

    if (ChanceDeRajada > 0.f)
    {
        GetWorldTimerManager().SetTimer(TimerChecagem, this, &ALampada::Checar, IntervaloChecagem, true);
    }
}

void ALampada::Checar()
{
    if (bForcadaApagada || bApagaoGeral) return;
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
    bAcesaAgora = bAcesa;
    Luz->SetVisibility(bAcesa && !bSemLuzReal);   // apagada = nao conta para luz nem sombra (evita o aviso do VSM)

    if (UMaterialInterface* Material = bAcesa ? MaterialAceso : MaterialApagado)
    {
        for (const int32 Slot : SlotsLuz)
        {
            Placa->SetMaterial(Slot, Material);
        }
    }

    // O zumbido "corta" quando a luz apaga (volume quase zero, sem parar o som)
    if (Zumbido && Zumbido->IsPlaying())
    {
        Zumbido->SetVolumeMultiplier((bAcesa && !bZumbidoMudo && !bSemZumbido && !bApagaoGeral) ? VolumeZumbido : 0.001f);
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

void ALampada::ForcarApagado(bool bApagar, bool bComEstalo)
{
    if (bQueimada) return;

    // Durante o apagao geral so guarda o pedido (ex.: um susto que estava terminando)
    if (bApagaoGeral)
    {
        bForcadaApagada = bApagar;
        return;
    }

    GetWorldTimerManager().ClearTimer(TimerRajada);
    PiscadasRestantes = 0;
    bForcadaApagada = bApagar;

    if (bComEstalo)
    {
        TocarEstalo(bApagar ? SonsApagar : SonsAcender);
    }
    DefinirAcesa(!bApagar);
}

void ALampada::DefinirZumbidoMudo(bool bMudo)
{
    bZumbidoMudo = bMudo;
    AtualizarVolumeZumbido();
}

void ALampada::DefinirSemZumbido(bool bNovo)
{
    bSemZumbido = bNovo;
    AtualizarVolumeZumbido();
}

void ALampada::AtualizarVolumeZumbido()
{
    if (Zumbido && Zumbido->IsPlaying())
    {
        Zumbido->SetVolumeMultiplier((bAcesaAgora && !bZumbidoMudo && !bSemZumbido && !bApagaoGeral) ? VolumeZumbido : 0.001f);
    }
}

void ALampada::DefinirApagaoGeral(bool bAtivo)
{
    if (bQueimada) return;

    bApagaoGeral = bAtivo;
    GetWorldTimerManager().ClearTimer(TimerRajada);
    PiscadasRestantes = 0;

    if (bAtivo)
    {
        bForcadaApagada = false;
        DefinirAcesa(false);
    }
    else
    {
        DefinirAcesa(!bForcadaApagada);
    }
    AtualizarVolumeZumbido();
}
