#include "EnigmaLevel0.h"
#include "QuadroLuz.h"
#include "ParedeSaida.h"
#include "Rabisco.h"
#include "LuzEmergencia.h"
#include "Lampada.h"
#include "DiretorSustos.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AEnigmaLevel0::AEnigmaLevel0()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    ClasseSeta = ALuzEmergencia::StaticClass();
    ClasseRabisco = ARabisco::StaticClass();
}

void AEnigmaLevel0::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority()) return;

    for (TActorIterator<AParedeSaida> It(GetWorld()); It; ++It)
    {
        Parede = *It;
        break;
    }

    // Decide ja (para nao aparecer quadro "sobrando") e confirma depois que todos entrarem
    EscolherGrupo();
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, this, &AEnigmaLevel0::EscolherGrupo, 3.f, false);
}

int32 AEnigmaLevel0::NumeroJogadores() const
{
    const AGameStateBase* Estado = GetWorld()->GetGameState();
    return Estado ? FMath::Max(1, Estado->PlayerArray.Num()) : 1;
}

void AEnigmaLevel0::EscolherGrupo()
{
    // 1-2 jogadores: quadros perto um do outro. 3-5: espalhados pelo mapa.
    const int32 N = NumeroJogadores();
    const int32 GrupoAtivo = (N <= 2) ? 0 : 1;

    Quadros.Reset();
    for (TActorIterator<AQuadroLuz> It(GetWorld()); It; ++It)
    {
        const bool bAtivo = (It->Grupo == GrupoAtivo);
        It->DefinirAtivo(bAtivo);
        if (bAtivo) Quadros.Add(*It);
    }

    const float PrimeiraDica = bModoTeste ? 30.f : (N == 1 ? TempoPrimeiraDicaSolo : TempoPrimeiraDicaGrupo);
    GetWorldTimerManager().SetTimer(TimerDica, this, &AEnigmaLevel0::DarDica, PrimeiraDica, false);

    UE_LOG(LogTemp, Log, TEXT("Enigma: %d jogador(es), grupo %d, %d quadros ativos"), N, GrupoAtivo, Quadros.Num());
}

void AEnigmaLevel0::QuadroDesligado(AQuadroLuz* Quadro)
{
    if (Fase != EFaseEnigma::Procurando) return;

    bAlgumProgresso = true;

    int32 Desligados = 0;
    for (AQuadroLuz* Q : Quadros)
    {
        if (Q && !Q->EstaLigado()) ++Desligados;
    }

    if (Desligados >= Quadros.Num())
    {
        GetWorldTimerManager().ClearTimer(TimerPrazo);
        EntrarEscuro();
        return;
    }

    // O primeiro desligado comeca a contagem: os outros precisam cair antes do prazo
    if (Desligados == 1)
    {
        const int32 N = NumeroJogadores();
        float Prazo = (N == 1) ? PrazoSolo : (N == 2 ? PrazoDupla : PrazoGrupo);
        if (bModoTeste) Prazo = 25.f;
        GetWorldTimerManager().SetTimer(TimerPrazo, this, &AEnigmaLevel0::PrazoEsgotado, Prazo, false);
    }
}

void AEnigmaLevel0::PrazoEsgotado()
{
    if (Fase != EFaseEnigma::Procurando) return;

    // Nao deu tempo: os disjuntores voltam sozinhos, com um tranco
    ReligarTodos();
    Multicast_Energia(true, false);
}

void AEnigmaLevel0::ReligarTodos()
{
    for (AQuadroLuz* Q : Quadros)
    {
        if (Q) Q->Religar();
    }
}

void AEnigmaLevel0::EntrarEscuro()
{
    Fase = EFaseEnigma::Escuro;
    PausarSustos(true);
    Multicast_Energia(false, true);

    if (Parede) Parede->DefinirRevelada(true);

    const float Duracao = bModoTeste ? 60.f : (NumeroJogadores() == 1 ? DuracaoEscuroSolo : DuracaoEscuroGrupo);
    GetWorldTimerManager().SetTimer(TimerEscuro, this, &AEnigmaLevel0::SairEscuro, Duracao, false);
}

void AEnigmaLevel0::SairEscuro()
{
    if (Fase != EFaseEnigma::Escuro) return;

    // Demorou demais: a energia volta e a saida some
    Fase = EFaseEnigma::Procurando;
    if (Parede) Parede->DefinirRevelada(false);
    ReligarTodos();
    Multicast_Energia(true, false);
    PausarSustos(false);
}

void AEnigmaLevel0::Concluir(APawn* Quem)
{
    if (Fase != EFaseEnigma::Escuro) return;

    Fase = EFaseEnigma::Concluido;
    GetWorldTimerManager().ClearTimer(TimerEscuro);
    GetWorldTimerManager().ClearTimer(TimerDica);

    UE_LOG(LogTemp, Log, TEXT("Enigma: %s atravessou a parede"), Quem ? *Quem->GetName() : TEXT("?"));

    Multicast_Final();
    GetWorldTimerManager().SetTimer(TimerViagem, this, &AEnigmaLevel0::Viajar, 2.5f, false);
}

void AEnigmaLevel0::Viajar()
{
    UWorld* Mundo = GetWorld();
    if (!Mundo) return;

    if (MapaSeguinte.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("EnigmaLevel0: MapaSeguinte nao foi escolhido no Blueprint"));
        return;
    }
    // Nome do pacote do mapa, ex.: /Game/Maps/Lvl_Level1
    const FString Mapa = MapaSeguinte.ToSoftObjectPath().GetLongPackageName();

    if (Mundo->GetNetMode() == NM_Standalone)
    {
        UGameplayStatics::OpenLevel(this, FName(*Mapa));
    }
    else
    {
        // Leva todo o grupo junto
        Mundo->ServerTravel(Mapa + TEXT("?listen"));
    }
}

void AEnigmaLevel0::PausarSustos(bool bPausar)
{
    for (TActorIterator<ADiretorSustos> It(GetWorld()); It; ++It)
    {
        It->DefinirPausado(bPausar);
    }
}

// ======================================================================
// Dicas: um rabisco novo aparece numa parede ATRAS do jogador
// ======================================================================
void AEnigmaLevel0::DarDica()
{
    if (Fase != EFaseEnigma::Procurando) return;

    // Se o grupo esta progredindo, espera mais
    if (bAlgumProgresso)
    {
        bAlgumProgresso = false;
        GetWorldTimerManager().SetTimer(TimerDica, this, &AEnigmaLevel0::DarDica, IntervaloDicas, false);
        return;
    }

    // Jogador aleatorio
    TArray<APawn*> Jogadores;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (APawn* P = PC->GetPawn()) Jogadores.Add(P);
        }
    }
    if (Jogadores.Num() == 0 || Quadros.Num() == 0)
    {
        GetWorldTimerManager().SetTimer(TimerDica, this, &AEnigmaLevel0::DarDica, 30.f, false);
        return;
    }
    APawn* Alvo = Jogadores[FMath::RandRange(0, Jogadores.Num() - 1)];

    // Quadro ligado mais proximo dele
    AQuadroLuz* MaisPerto = nullptr;
    float Melhor = TNumericLimits<float>::Max();
    for (AQuadroLuz* Q : Quadros)
    {
        if (!Q || !Q->EstaLigado()) continue;
        const float D = FVector::DistSquared(Q->GetActorLocation(), Alvo->GetActorLocation());
        if (D < Melhor) { Melhor = D; MaisPerto = Q; }
    }

    if (MaisPerto)
    {
        float MeiaAltura = 90.f;
        if (const ACharacter* C = Cast<ACharacter>(Alvo))
        {
            MeiaAltura = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        }
        const float AlturaRabisco = Alvo->GetActorLocation().Z - MeiaAltura + 150.f;
        const FVector Atras = -Alvo->GetBaseAimRotation().Vector().GetSafeNormal2D();

        FCollisionQueryParams Params(SCENE_QUERY_STAT(DicaEnigma), false, Alvo);
        for (int32 Tentativa = 0; Tentativa < 8; ++Tentativa)
        {
            const FVector Dir = Atras.RotateAngleAxis(FMath::FRandRange(-70.f, 70.f), FVector::UpVector);
            const FVector Inicio(Alvo->GetActorLocation().X, Alvo->GetActorLocation().Y, AlturaRabisco);

            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, Inicio, Inicio + Dir * 800.f, ECC_Visibility, Params)) continue;
            if (FMath::Abs(Hit.ImpactNormal.Z) > 0.3f || Hit.Distance < 150.f) continue;

            const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
            const FRotator Rot = Normal.Rotation();

            // Seta apontando para o quadro, do ponto de vista de quem le a parede
            const FVector DireitaLeitor = FRotationMatrix((-Normal).Rotation()).GetScaledAxis(EAxis::Y);
            const bool bDireita = FVector::DotProduct(MaisPerto->GetActorLocation() - Hit.ImpactPoint, DireitaLeitor) > 0.f;
            const FString Texto = bDireita ? TEXT("SIGA -->") : TEXT("<-- SIGA");

            Multicast_Rabisco(Hit.ImpactPoint + Normal * 0.6f, Rot, Texto);
            break;
        }
    }

    GetWorldTimerManager().SetTimer(TimerDica, this, &AEnigmaLevel0::DarDica, bModoTeste ? 30.f : IntervaloDicas, false);
}

// ======================================================================
// Efeitos em todos os jogadores
// ======================================================================
void AEnigmaLevel0::Multicast_Energia_Implementation(bool bLigada, bool bComRota)
{
    // Trava geral: nenhum susto ou piscada consegue religar nada enquanto estiver escuro
    for (TActorIterator<ALampada> It(GetWorld()); It; ++It)
    {
        It->DefinirApagaoGeral(!bLigada);
    }

    // So mostra a rota quando a saida esta aberta (no prazo esgotado a energia so "volta")
    MostrarRotaDeFuga(!bLigada && bComRota);

    // O "TUM" da energia caindo/voltando no predio inteiro
    if (SomEnergia)
    {
        UGameplayStatics::PlaySound2D(this, SomEnergia, 1.6f, bLigada ? 0.7f : 0.4f);
    }
}

void AEnigmaLevel0::Multicast_Rabisco_Implementation(FVector_NetQuantize Local, FRotator Rotacao, const FString& Texto)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ARabisco* R = GetWorld()->SpawnActor<ARabisco>(ClasseRabisco ? ClasseRabisco.Get() : ARabisco::StaticClass(), Local, Rotacao, Params))
    {
        R->Configurar(FText::FromString(Texto), 15.f, FColor(25, 25, 45));
    }
}

void AEnigmaLevel0::Multicast_Final_Implementation()
{
    // Tudo some num estalo e silencio
    for (TActorIterator<ALampada> It(GetWorld()); It; ++It)
    {
        It->DefinirApagaoGeral(true);
    }
    MostrarRotaDeFuga(false);
    if (SomEnergia)
    {
        UGameplayStatics::PlaySound2D(this, SomEnergia, 2.f, 0.25f);
    }
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 2.f, FLinearColor::Black, false, true);
        }
    }
}

// ======================================================================
// Rota de fuga: setas verdes no teto, da celula do jogador ate a saida
// ======================================================================
void AEnigmaLevel0::MostrarRotaDeFuga(bool bMostrar)
{
    for (ALuzEmergencia* Seta : SetasEmergencia)
    {
        if (IsValid(Seta)) Seta->Destroy();
    }
    SetasEmergencia.Reset();

    if (!bMostrar || MapaColunas <= 0 || MapaLinhas <= 0 || CaminhoSaida.Num() != MapaColunas * MapaLinhas) return;

    // Cada maquina mostra o caminho do SEU jogador (no servidor dedicado nao ha jogador local)
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    APawn* Eu = PC ? PC->GetPawn() : nullptr;
    if (!Eu || !Eu->IsLocallyControlled()) return;

    auto Celula = [this](int32 Indice)
    {
        return FIntPoint(Indice % MapaColunas, Indice / MapaColunas);
    };
    auto Centro = [this](const FIntPoint& P, float Z)
    {
        return MapaOrigem.TransformPosition(FVector(P.X * MapaCelula, P.Y * MapaCelula, Z));
    };

    const FVector Local = MapaOrigem.InverseTransformPosition(Eu->GetActorLocation());
    const int32 X = FMath::Clamp(FMath::RoundToInt(Local.X / MapaCelula), 0, MapaColunas - 1);
    const int32 Y = FMath::Clamp(FMath::RoundToInt(Local.Y / MapaCelula), 0, MapaLinhas - 1);

    int32 Atual = X + Y * MapaColunas;
    int32 Passos = 0;
    int32 DirAnterior = -1;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 Seguranca = 0; Seguranca < CaminhoSaida.Num(); ++Seguranca)
    {
        const int32 Proxima = CaminhoSaida.IsValidIndex(Atual) ? CaminhoSaida[Atual] : -1;
        if (Proxima < 0) break;   // chegou na celula da saida

        const FIntPoint A = Celula(Atual);
        const FIntPoint B = Celula(Proxima);
        const int32 Dir = (B.X != A.X) ? (B.X > A.X ? 0 : 1) : (B.Y > A.Y ? 2 : 3);

        // Uma seta a cada N celulas, e sempre que o caminho vira
        if (Passos % FMath::Max(1, EspacoSetas) == 0 || Dir != DirAnterior)
        {
            const FVector De = Centro(A, MapaAltura - 6.f);
            const FVector Para = Centro(B, MapaAltura - 6.f);
            const FRotator Rot = (Para - De).Rotation();
            if (ALuzEmergencia* Seta = GetWorld()->SpawnActor<ALuzEmergencia>(ClasseSeta ? ClasseSeta.Get() : ALuzEmergencia::StaticClass(), De, Rot, Params))
            {
                SetasEmergencia.Add(Seta);
            }
        }

        DirAnterior = Dir;
        Atual = Proxima;
        ++Passos;
    }
}
