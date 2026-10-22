#include "QuadroLuz.h"
#include "EnigmaLevel0.h"
#include "Lampada.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/ConstructorHelpers.h"

// Medidas (cm). O ator fica no chao, encostado na parede, com +X apontando para fora dela.
namespace QuadroMedidas
{
    const FVector TamanhoCaixa(12.f, 45.f, 65.f);
    const float AlturaCentro = 140.f;
    const float TopoCaixa = AlturaCentro + 32.5f;
    const float LadoConduite = 5.f;
    const float YConduite = 12.f;
}

AQuadroLuz::AQuadroLuz()
{
    using namespace QuadroMedidas;

    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    // Materiais e sons sao escolhidos no BP_QuadroLuz

    auto CriarCubo = [&](const TCHAR* Nome, USceneComponent* Pai)
    {
        UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
        C->SetupAttachment(Pai);
        if (Cubo.Succeeded()) C->SetStaticMesh(Cubo.Object);
        return C;
    };

    // Caixa metalica
    Caixa = CriarCubo(TEXT("Caixa"), RootComponent);
    Caixa->SetRelativeLocation(FVector(TamanhoCaixa.X * 0.5f, 0.f, AlturaCentro));
    Caixa->SetRelativeScale3D(TamanhoCaixa / 100.f);

    // Alavanca (gira no pivo: para cima = ligado, para baixo = desligado)
    Pivo = CreateDefaultSubobject<USceneComponent>(TEXT("Pivo"));
    Pivo->SetupAttachment(RootComponent);
    Pivo->SetRelativeLocation(FVector(TamanhoCaixa.X, -10.f, AlturaCentro));

    Alavanca = CriarCubo(TEXT("Alavanca"), Pivo);
    Alavanca->SetRelativeLocation(FVector(7.f, 0.f, 0.f));
    Alavanca->SetRelativeScale3D(FVector(0.14f, 0.035f, 0.035f));

    // Luzinha de "energia"
    Indicador = CriarCubo(TEXT("Indicador"), RootComponent);
    Indicador->SetRelativeLocation(FVector(TamanhoCaixa.X + 0.5f, 12.f, AlturaCentro + 20.f));
    Indicador->SetRelativeScale3D(FVector(0.02f, 0.025f, 0.025f));
    Indicador->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Conduite: sobe da caixa ate o forro e anda pelo teto
    ConduiteSubida = CriarCubo(TEXT("ConduiteSubida"), RootComponent);
    ConduiteSubida->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ConduiteTeto = CriarCubo(TEXT("ConduiteTeto"), RootComponent);
    ConduiteTeto->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Etiqueta pequena na tampa
    Etiqueta = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Etiqueta"));
    Etiqueta->SetupAttachment(RootComponent);
    Etiqueta->SetRelativeLocation(FVector(TamanhoCaixa.X + 0.3f, 0.f, AlturaCentro - 20.f));
    Etiqueta->SetHorizontalAlignment(EHTA_Center);
    Etiqueta->SetVerticalAlignment(EVRTA_TextCenter);
    Etiqueta->SetWorldSize(3.f);
    Etiqueta->SetTextRenderColor(FColor(30, 30, 30));
    Etiqueta->SetText(FText::FromString(TEXT("QUADRO GERAL")));

    // Aviso vermelho de alta tensao, como nos quadros de verdade
    UTextRenderComponent* Aviso = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Aviso"));
    Aviso->SetupAttachment(RootComponent);
    Aviso->SetRelativeLocation(FVector(TamanhoCaixa.X + 0.3f, 9.f, AlturaCentro + 2.f));
    Aviso->SetHorizontalAlignment(EHTA_Center);
    Aviso->SetVerticalAlignment(EVRTA_TextCenter);
    Aviso->SetWorldSize(2.4f);
    Aviso->SetTextRenderColor(FColor(165, 22, 18));
    Aviso->SetText(FText::FromString(TEXT("PERIGO\nALTA TENS\u00C3O\n220V")));
    Aviso->SetCastShadow(false);

    // Parafusos nos cantos da tampa
    for (int32 I = 0; I < 4; ++I)
    {
        UStaticMeshComponent* Parafuso = CriarCubo(*FString::Printf(TEXT("Parafuso%d"), I), RootComponent);
        const float Y = (I % 2 == 0) ? -19.f : 19.f;
        const float Z = AlturaCentro + ((I < 2) ? 28.5f : -28.5f);
        Parafuso->SetRelativeLocation(FVector(TamanhoCaixa.X + 0.4f, Y, Z));
        Parafuso->SetRelativeScale3D(FVector(0.008f, 0.012f, 0.012f));
        Parafuso->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Parafuso->SetCastShadow(false);
    }

    // Rabisco de caneta na parede, ao lado da caixa
    Rabisco = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Rabisco"));
    Rabisco->SetupAttachment(RootComponent);
    Rabisco->SetRelativeLocation(FVector(0.6f, -45.f, AlturaCentro + 10.f));
    Rabisco->SetHorizontalAlignment(EHTA_Center);
    Rabisco->SetVerticalAlignment(EVRTA_TextCenter);
    Rabisco->SetWorldSize(16.f);
    Rabisco->SetTextRenderColor(FColor(25, 25, 45));

    // Dica "[E]" ao lado da alavanca (so aparece quando voce mira)
    Dica = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Dica"));
    Dica->SetupAttachment(RootComponent);
    Dica->SetRelativeLocation(FVector(TamanhoCaixa.X + 0.4f, -10.f, AlturaCentro + 16.f));
    Dica->SetHorizontalAlignment(EHTA_Center);
    Dica->SetVerticalAlignment(EVRTA_TextCenter);
    Dica->SetWorldSize(4.f);
    Dica->SetTextRenderColor(FColor(235, 235, 220));
    Dica->SetText(FText::FromString(TEXT("[E]")));
    Dica->SetHiddenInGame(true);
    Dica->SetVisibility(false);

    // Chiado eletrico (o zumbido da lampada, bem mais agudo e baixo)
    Chiado = CreateDefaultSubobject<UAudioComponent>(TEXT("Chiado"));
    Chiado->SetupAttachment(Caixa);
    Chiado->bAutoActivate = false;
    Chiado->SetPitchMultiplier(2.2f);
    Chiado->SetVolumeMultiplier(0.25f);
}

void AQuadroLuz::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    using namespace QuadroMedidas;

    const float Subida = FMath::Max(1.f, AlturaTeto - TopoCaixa);
    ConduiteSubida->SetRelativeLocation(FVector(LadoConduite * 0.5f, YConduite, TopoCaixa + Subida * 0.5f));
    ConduiteSubida->SetRelativeScale3D(FVector(LadoConduite, LadoConduite, Subida) / 100.f);

    const float Comp = FMath::Max(1.f, ComprimentoConduite);
    ConduiteTeto->SetRelativeLocation(FVector(LadoConduite * 0.5f, YConduite + Comp * 0.5f, AlturaTeto - LadoConduite * 0.5f));
    ConduiteTeto->SetRelativeScale3D(FVector(LadoConduite, Comp, LadoConduite) / 100.f);

    Rabisco->SetText(TextoRabisco);

    // Pinta todas as pecas de metal (menos a luzinha) com o material escolhido no Blueprint
    if (MaterialMetal)
    {
        TArray<UStaticMeshComponent*> Pecas;
        GetComponents<UStaticMeshComponent>(Pecas);
        for (UStaticMeshComponent* Peca : Pecas)
        {
            if (Peca != Indicador) Peca->SetMaterial(0, MaterialMetal);
        }
    }
    AtualizarAlavanca(false);
}

void AQuadroLuz::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AQuadroLuz, bLigado);
    DOREPLIFETIME(AQuadroLuz, bAtivo);
}

void AQuadroLuz::BeginPlay()
{
    Super::BeginPlay();
    if (SomChiado) Chiado->SetSound(SomChiado);
    AtualizarAlavanca(false);
    AplicarAtivo();
}

void AQuadroLuz::Interagir(APawn* Quem)
{
    if (!HasAuthority() || !PodeInteragir()) return;

    bLigado = false;
    AtualizarAlavanca(true);   // no servidor o OnRep nao roda sozinho

    for (TActorIterator<AEnigmaLevel0> It(GetWorld()); It; ++It)
    {
        It->QuadroDesligado(this);
        break;
    }
}

void AQuadroLuz::Religar()
{
    if (!HasAuthority() || bLigado) return;
    bLigado = true;
    AtualizarAlavanca(true);
}

void AQuadroLuz::DefinirAtivo(bool bNovo)
{
    if (!HasAuthority()) return;
    bAtivo = bNovo;
    AplicarAtivo();
}

void AQuadroLuz::OnRep_Ligado()
{
    AtualizarAlavanca(true);
}

void AQuadroLuz::OnRep_Ativo()
{
    AplicarAtivo();
}

void AQuadroLuz::AtualizarAlavanca(bool bComSom)
{
    Pivo->SetRelativeRotation(FRotator(bLigado ? 40.f : -40.f, 0.f, 0.f));

    if (UMaterialInterface* M = bLigado ? MaterialIndicadorLigado : MaterialIndicadorDesligado)
    {
        Indicador->SetMaterial(0, M);
    }

    if (Chiado && HasActorBegunPlay())
    {
        if (bLigado && bAtivo && SomChiado)
        {
            if (!Chiado->IsPlaying()) Chiado->Play(FMath::FRandRange(0.f, 1.f));
        }
        else
        {
            Chiado->Stop();
        }
    }

    if (bComSom && SomAlavanca && HasActorBegunPlay())
    {
        // Estalo grave e pesado: um disjuntor velho
        UGameplayStatics::PlaySoundAtLocation(this, SomAlavanca, Caixa->GetComponentLocation(), 1.4f, 0.5f);
    }
}

void AQuadroLuz::AplicarAtivo()
{
    SetActorHiddenInGame(!bAtivo);
    SetActorEnableCollision(bAtivo);

    if (LampadaSilenciosa)
    {
        LampadaSilenciosa->DefinirSemZumbido(bAtivo);
    }
    AtualizarAlavanca(false);
}

void AQuadroLuz::DefinirDestaque(bool bDestacado)
{
    const bool bMostrar = bDestacado && bAtivo && bLigado;
    Dica->SetHiddenInGame(!bMostrar);
    Dica->SetVisibility(bMostrar);
}
