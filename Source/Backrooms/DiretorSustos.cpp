#include "DiretorSustos.h"
#include "Lampada.h"
#include "SombraCorredor.h"
#include "AmbienteLevel0.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "TimerManager.h"

ADiretorSustos::ADiretorSustos()
{
    PrimaryActorTick.bCanEverTick = false;

    // Precisa existir em todos os jogadores para receber os sustos
    bReplicates = true;
    bAlwaysRelevant = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    // Sons (passos, sombra, batidas) sao escolhidos no Blueprint (BP_DiretorSustos)
    ClasseSombra = ASombraCorredor::StaticClass();
}

void ADiretorSustos::BeginPlay()
{
    Super::BeginPlay();

    // So o servidor decide os sustos
    if (!HasAuthority()) return;

    // Espera alguem dar o primeiro passo antes de comecar a contar
    GetWorldTimerManager().SetTimer(TimerEsperaPasso, this, &ADiretorSustos::VerificarPrimeiroPasso, 0.5f, true);
}

void ADiretorSustos::VerificarPrimeiroPasso()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        const APawn* P = PC ? PC->GetPawn() : nullptr;
        if (P && P->GetVelocity().Size2D() > 50.f)
        {
            GetWorldTimerManager().ClearTimer(TimerEsperaPasso);
            const float Atraso = bModoTeste ? 5.f : AtrasoAposPrimeiroPasso;
            Agendar(Atraso);
            UE_LOG(LogTemp, Log, TEXT("DiretorSustos: primeiro passo detectado, sustos em %.0f s"), Atraso);
            return;
        }
    }
}

void ADiretorSustos::Agendar(float Atraso)
{
    GetWorldTimerManager().SetTimer(TimerProximo, this, &ADiretorSustos::Proximo, FMath::Max(0.1f, Atraso), false);
    UE_LOG(LogTemp, Log, TEXT("DiretorSustos: proximo susto em %.0f s"), Atraso);
}

void ADiretorSustos::Proximo()
{
    if (bPausado)
    {
        // Seguranca: nunca fica pausado para sempre
        TempoPausado += 15.f;
        if (TempoPausado < 180.f)
        {
            UE_LOG(LogTemp, Log, TEXT("DiretorSustos: pausado (enigma no escuro)"));
            Agendar(15.f);
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("DiretorSustos: pausado ha muito tempo, voltando sozinho"));
        bPausado = false;
    }
    TempoPausado = 0.f;

    const ETipoSusto Tipo = bPrimeiro ? PrimeiroSusto : Sortear();
    bPrimeiro = false;

    DispararSusto(Tipo);

    float Intervalo;
    if (bModoTeste)
    {
        Intervalo = FMath::FRandRange(6.f, 10.f);
    }
    else
    {
        Intervalo = FMath::Max(IntervaloMinimo, FMath::FRandRange(IntervaloEntre.X, IntervaloEntre.Y) * FatorTensao);
        FatorTensao *= AceleracaoTensao;
    }
    Agendar(Intervalo);
}

ETipoSusto ADiretorSustos::Sortear() const
{
    // Nunca repete o mesmo susto duas vezes seguidas
    const bool bTemBatidas = SomBatidas != nullptr;
    float Pesos[5] = { PesoApagao, SonsPassos.Num() > 0 ? PesoPassos : 0.f, PesoSilencio, PesoPiscada, bTemBatidas ? PesoBatidas : 0.f };
    Pesos[static_cast<int32>(Ultimo)] = 0.f;

    float Total = 0.f;
    for (float P : Pesos) Total += P;
    if (Total <= 0.f) return ETipoSusto::Apagao;

    float R = FMath::FRand() * Total;
    for (int32 I = 0; I < 5; ++I)
    {
        if (R < Pesos[I]) return static_cast<ETipoSusto>(I);
        R -= Pesos[I];
    }
    return ETipoSusto::Apagao;
}

APawn* ADiretorSustos::EscolherAlvo() const
{
    TArray<APawn*> Jogadores;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (APawn* P = PC->GetPawn())
            {
                Jogadores.Add(P);
            }
        }
    }
    return Jogadores.Num() > 0 ? Jogadores[FMath::RandRange(0, Jogadores.Num() - 1)] : nullptr;
}

void ADiretorSustos::DispararSusto(ETipoSusto Tipo)
{
    if (!HasAuthority()) return;

    APawn* Alvo = EscolherAlvo();
    if (!Alvo)
    {
        UE_LOG(LogTemp, Warning, TEXT("DiretorSustos: nenhum jogador para assustar"));
        return;
    }

    Ultimo = Tipo;
    const FVector Centro = Alvo->GetActorLocation();

    switch (Tipo)
    {
    case ETipoSusto::Apagao:
    {
        // A sombra vem sempre no primeiro apagao; depois, so as vezes
        const bool bPrimeira = !bJaTeveSombra;
        const bool bSombra = bPrimeira || FMath::FRand() < ChanceSombra;
        bJaTeveSombra = true;
        Multicast_Apagao(Centro, FMath::FRandRange(DuracaoApagao.X, DuracaoApagao.Y), bSombra, bPrimeira);
    }
        break;
    case ETipoSusto::Piscada:
        Multicast_Piscada(Centro);
        break;
    case ETipoSusto::Silencio:
        Multicast_Silencio(Centro, FMath::FRandRange(DuracaoSilencio.X, DuracaoSilencio.Y));
        break;
    case ETipoSusto::Passos:
        IniciarPassos(Alvo);
        break;
    case ETipoSusto::Batidas:
    {
        FVector Local;
        if (AcharParedeAoLado(Alvo, Local))
        {
            Multicast_Batidas(Local);
        }
        else
        {
            Multicast_Piscada(Centro);   // sem parede perto: troca por uma piscada
        }
    }
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("DiretorSustos: susto %d em %s"), static_cast<int32>(Tipo), *Alvo->GetName());
}

// ======================================================================
// Passos atras do jogador
// ======================================================================
void ADiretorSustos::IniciarPassos(APawn* Alvo)
{
    if (SonsPassos.Num() == 0) return;

    AlvoPassos = Alvo;

    // Nasce no chao, atras de quem esta olhando para frente
    const FVector Frente = Alvo->GetBaseAimRotation().Vector().GetSafeNormal2D();
    float MeiaAltura = 90.f;
    if (const ACharacter* Personagem = Cast<ACharacter>(Alvo))
    {
        MeiaAltura = Personagem->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }

    PosicaoPassos = Alvo->GetActorLocation() - Frente * FMath::FRandRange(DistanciaPassos.X, DistanciaPassos.Y);
    PosicaoPassos.Z = Alvo->GetActorLocation().Z - MeiaAltura + 5.f;
    PassosRestantes = FMath::RandRange(NumeroPassos.X, FMath::Max(NumeroPassos.X, NumeroPassos.Y));

    GetWorldTimerManager().SetTimer(TimerPassos, this, &ADiretorSustos::DarPasso, IntervaloPassos, true, IntervaloPassos);
}

void ADiretorSustos::DarPasso()
{
    APawn* Alvo = AlvoPassos.Get();
    if (!Alvo || PassosRestantes <= 0)
    {
        GetWorldTimerManager().ClearTimer(TimerPassos);
        return;
    }

    // Virou para olhar? Os passos param na hora (nao tem ninguem la... ou tem?)
    const FVector ParaPassos = (PosicaoPassos - Alvo->GetPawnViewLocation()).GetSafeNormal2D();
    const FVector Visao = Alvo->GetBaseAimRotation().Vector().GetSafeNormal2D();
    if (FVector::DotProduct(ParaPassos, Visao) > 0.45f)
    {
        GetWorldTimerManager().ClearTimer(TimerPassos);
        return;
    }

    // Chega mais perto, mas nunca encosta
    FVector ParaAlvo = Alvo->GetActorLocation() - PosicaoPassos;
    ParaAlvo.Z = 0.f;
    const float Distancia = ParaAlvo.Size();
    if (Distancia < 250.f)
    {
        GetWorldTimerManager().ClearTimer(TimerPassos);
        return;
    }

    PosicaoPassos += ParaAlvo.GetSafeNormal() * FMath::Min(ComprimentoPasso, Distancia - 250.f);
    Multicast_Passo(PosicaoPassos);
    --PassosRestantes;
}

void ADiretorSustos::Multicast_Passo_Implementation(FVector_NetQuantize Local)
{
    if (SonsPassos.Num() == 0) return;

    USoundBase* Som = SonsPassos[FMath::RandRange(0, SonsPassos.Num() - 1)];
    if (!Som) return;

    UGameplayStatics::PlaySoundAtLocation(this, Som, Local,
        VolumePassos * FMath::FRandRange(0.85f, 1.f), FMath::FRandRange(0.88f, 1.02f), 0.f, AtenuacaoPassos);
}

// ======================================================================
// Lampadas
// ======================================================================
TArray<ALampada*> ADiretorSustos::LampadasPerto(const FVector& Centro, float Raio) const
{
    TArray<ALampada*> Resultado;
    const float Raio2 = Raio * Raio;
    for (TActorIterator<ALampada> It(GetWorld()); It; ++It)
    {
        if (!It->EstaQueimada() && FVector::DistSquared(It->GetActorLocation(), Centro) <= Raio2)
        {
            Resultado.Add(*It);
        }
    }
    // Da mais longe para a mais perto
    Resultado.Sort([Centro](const ALampada& A, const ALampada& B)
    {
        return FVector::DistSquared(A.GetActorLocation(), Centro) > FVector::DistSquared(B.GetActorLocation(), Centro);
    });
    return Resultado;
}

void ADiretorSustos::AgendarLampada(ALampada* Lampada, float Atraso, bool bApagar, bool bComEstalo)
{
    TWeakObjectPtr<ALampada> Fraca(Lampada);
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Fraca, bApagar, bComEstalo]()
    {
        if (ALampada* L = Fraca.Get())
        {
            L->ForcarApagado(bApagar, bComEstalo);
        }
    }), FMath::Max(0.01f, Atraso), false);
}

void ADiretorSustos::Multicast_Apagao_Implementation(FVector_NetQuantize Centro, float Duracao, bool bComSombra, bool bPrimeiraSombra)
{
    // A escuridao vem do fundo do corredor ate o jogador
    const TArray<ALampada*> Lista = LampadasPerto(Centro, RaioApagao);
    const int32 N = Lista.Num();
    if (N == 0) return;

    // A cascata inteira dura no maximo 1,6 s, por maior que seja a area
    const float Passo = FMath::Min(TempoCascata, 1.6f / N);
    const float FimCascata = N * Passo;

    for (int32 I = 0; I < N; ++I)
    {
        const bool bPerto = I >= N - 6;   // so as mais proximas estalam (evita barulho demais)
        AgendarLampada(Lista[I], I * Passo, true, bPerto);

        // Voltam quase juntas, num tranco
        AgendarLampada(Lista[I], FimCascata + Duracao + FMath::FRandRange(0.f, 0.35f), false, bPerto);
    }

    // Logo que a luz volta: algo atravessa o corredor que voce esta olhando
    if (bComSombra)
    {
        TentativasSombra = 0;
        bSombraPrimeira = bPrimeiraSombra;
        GetWorldTimerManager().SetTimer(TimerSombra, this, &ADiretorSustos::TentarSombra, 0.15f, true, FimCascata + Duracao + 0.45f);
    }
}

void ADiretorSustos::Multicast_Piscada_Implementation(FVector_NetQuantize Centro)
{
    // Falha na rede eletrica: tudo pisca junto, num ritmo irregular
    const TArray<ALampada*> Lista = LampadasPerto(Centro, RaioPiscada);
    const float Tempos[] = { 0.f, 0.09f, 0.21f, 0.27f, 0.52f, 0.6f, 0.95f };
    const int32 NumTempos = UE_ARRAY_COUNT(Tempos);

    for (int32 I = 0; I < Lista.Num(); ++I)
    {
        const bool bPerto = I >= Lista.Num() - 4;
        for (int32 K = 0; K < NumTempos; ++K)
        {
            const bool bApagar = (K % 2 == 0);
            AgendarLampada(Lista[I], Tempos[K], bApagar, bPerto && (K == 0 || K == NumTempos - 1));
        }
    }
    // Termina acesa: o ultimo tempo e par (apaga), entao acende logo depois
    for (ALampada* L : Lista)
    {
        AgendarLampada(L, Tempos[NumTempos - 1] + 0.12f, false, false);
    }
}

void ADiretorSustos::Multicast_Silencio_Implementation(FVector_NetQuantize Centro, float Duracao)
{
    // O zumbido que nunca para... para. Silencio absoluto (ate o ar do predio some).
    for (TActorIterator<AAmbienteLevel0> It(GetWorld()); It; ++It)
    {
        It->Abafar(0.05f, 0.3f);
        TWeakObjectPtr<AAmbienteLevel0> Amb(*It);
        FTimerHandle HAmb;
        GetWorldTimerManager().SetTimer(HAmb, FTimerDelegate::CreateWeakLambda(this, [Amb]()
        {
            if (Amb.IsValid()) Amb->Abafar(1.f, 2.5f);
        }), FMath::Max(0.1f, Duracao), false);
    }

    TArray<TWeakObjectPtr<ALampada>> Todas;
    for (TActorIterator<ALampada> It(GetWorld()); It; ++It)
    {
        It->DefinirZumbidoMudo(true);
        Todas.Add(*It);
    }

    // Volta com um estalo na lampada mais proxima do jogador
    TWeakObjectPtr<ALampada> MaisPerto;
    const TArray<ALampada*> Perto = LampadasPerto(Centro, 1500.f);
    if (Perto.Num() > 0)
    {
        MaisPerto = Perto.Last();
    }

    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Todas, MaisPerto]()
    {
        for (const TWeakObjectPtr<ALampada>& L : Todas)
        {
            if (L.IsValid()) L->DefinirZumbidoMudo(false);
        }
        if (ALampada* L = MaisPerto.Get())
        {
            L->ForcarApagado(true, true);
            AgendarLampada(L, 0.15f, false, true);
        }
    }), FMath::Max(0.1f, Duracao), false);
}

void ADiretorSustos::DefinirPausado(bool bNovo)
{
    bPausado = bNovo;
    TempoPausado = 0.f;
    UE_LOG(LogTemp, Log, TEXT("DiretorSustos: %s"), bPausado ? TEXT("pausado") : TEXT("liberado"));
    if (bPausado)
    {
        GetWorldTimerManager().ClearTimer(TimerPassos);
    }
}

// ======================================================================
// Sombra: aparicao no fundo do corredor para onde o jogador esta olhando
// ======================================================================
void ADiretorSustos::TentarSombra()
{
    // Se o jogador estiver de cara para a parede, espera ele olhar para um corredor:
    // na primeira vez ate ~10 s (ela e importante), nas outras ~2,5 s
    const int32 Limite = bSombraPrimeira ? 66 : 16;
    if (CriarSombra())
    {
        GetWorldTimerManager().ClearTimer(TimerSombra);
        return;
    }
    if (++TentativasSombra > Limite)
    {
        GetWorldTimerManager().ClearTimer(TimerSombra);
        // A primeira nao aconteceu: guarda para o proximo apagao
        if (bSombraPrimeira && HasAuthority())
        {
            bJaTeveSombra = false;
        }
        UE_LOG(LogTemp, Log, TEXT("DiretorSustos: sombra sem corredor livre, fica para o proximo apagao"));
    }
}

bool ADiretorSustos::CriarSombra()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    APawn* Eu = PC ? PC->GetPawn() : nullptr;
    if (!Eu || !Eu->IsLocallyControlled()) return true;   // servidor dedicado: nada a fazer

    FVector Olho;
    FRotator Rot;
    PC->GetPlayerViewPoint(Olho, Rot);
    const FVector Frente = Rot.Vector().GetSafeNormal2D();
    if (Frente.IsNearlyZero()) return false;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(SombraCorredor), false, Eu);

    // Ate onde o corredor vai na direcao do olhar
    const FVector OlhoPlano(Olho.X, Olho.Y, Olho.Z);
    FHitResult Hit;
    const float Alcance = 2400.f;
    const float Profundidade = GetWorld()->LineTraceSingleByChannel(Hit, OlhoPlano, OlhoPlano + Frente * Alcance, ECC_Visibility, Params)
        ? Hit.Distance : Alcance;
    if (Profundidade < 600.f) return false;   // parede perto demais

    float MeiaAltura = 90.f;
    if (const ACharacter* C = Cast<ACharacter>(Eu))
    {
        MeiaAltura = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }
    const float Chao = Eu->GetActorLocation().Z - MeiaAltura;

    // Vezes seguintes: as vezes ela esta PARADA no fim do corredor, olhando
    if (!bSombraPrimeira && Profundidade >= 1300.f && FMath::FRand() < ChanceVulto)
    {
        if (CriarVulto(Eu, OlhoPlano, Frente, Profundidade, Chao)) return true;
    }

    // Ponto no fundo do corredor (longe, mas antes da parede)
    const float Distancia = FMath::FRandRange(450.f, FMath::Max(450.f, FMath::Min(Profundidade - 150.f, 1800.f)));
    const FVector Ponto = FVector(OlhoPlano.X, OlhoPlano.Y, Chao + 100.f) + Frente * Distancia;

    // Largura do corredor naquele ponto: de uma parede a outra
    const FVector Lado = FVector::CrossProduct(FVector::UpVector, Frente);
    auto Medir = [&](const FVector& Dir)
    {
        FHitResult H;
        return GetWorld()->LineTraceSingleByChannel(H, Ponto, Ponto + Dir * 500.f, ECC_Visibility, Params) ? H.Distance : 500.f;
    };
    const float Esquerda = Medir(-Lado);
    const float Direita = Medir(Lado);
    if (Esquerda + Direita < 150.f) return false;

    // Sai de dentro de uma parede e entra na outra
    const bool bDaEsquerda = FMath::RandBool();
    const FVector A = Ponto - Lado * (Esquerda + 30.f);
    const FVector B = Ponto + Lado * (Direita + 30.f);
    FVector De = bDaEsquerda ? A : B;
    FVector Para = bDaEsquerda ? B : A;
    De.Z = Chao;
    Para.Z = Chao;

    FActorSpawnParameters SP;
    SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ASombraCorredor* Sombra = GetWorld()->SpawnActor<ASombraCorredor>(ClasseSombra ? ClasseSombra.Get() : ASombraCorredor::StaticClass(), De, FRotator::ZeroRotator, SP);
    if (!Sombra) return false;

    const float Duracao = FMath::Clamp((Esquerda + Direita + 60.f) / FMath::Max(100.f, VelocidadeSombra), 0.25f, 0.7f);
    Sombra->Iniciar(De, Para, Duracao);
    EfeitosTravessia(De, Para, Duracao, Eu);

    if (USoundBase* Som = SomSombra.Get())
    {
        UGameplayStatics::PlaySound2D(this, Som, VolumeSombra);
    }

    UE_LOG(LogTemp, Log, TEXT("DiretorSustos: sombra a %.0f cm"), Distancia);
    return true;
}

void ADiretorSustos::AgendarLocal(float Atraso, TFunction<void()> Acao)
{
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Acao]() { Acao(); }), FMath::Max(0.01f, Atraso), false);
}

// Quando ela passa: as lampadas em cima dela falham, o zumbido some por um instante
// e da para ouvir os passos rapidos no carpete.
void ADiretorSustos::EfeitosTravessia(const FVector& De, const FVector& Para, float Duracao, APawn* Eu)
{
    const FVector Meio = (De + Para) * 0.5f;

    // Lampadas logo acima do caminho piscam quando ela passa por baixo
    for (ALampada* L : LampadasPerto(Meio, 450.f))
    {
        TWeakObjectPtr<ALampada> Fraca(L);
        AgendarLocal(Duracao * 0.4f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(true, false); });
        AgendarLocal(Duracao * 0.4f + 0.11f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(false, true); });
    }

    // O zumbido em volta do jogador some enquanto ela passa... e volta
    TArray<TWeakObjectPtr<ALampada>> Perto;
    for (ALampada* L : LampadasPerto(Eu->GetActorLocation(), 2200.f))
    {
        L->DefinirZumbidoMudo(true);
        Perto.Add(L);
    }
    AgendarLocal(1.4f, [Perto]()
    {
        for (const TWeakObjectPtr<ALampada>& L : Perto)
        {
            if (L.IsValid()) L->DefinirZumbidoMudo(false);
        }
    });

    // Tres passos rapidos no carpete
    if (SonsPassos.Num() > 0)
    {
        for (int32 I = 0; I < 3; ++I)
        {
            const FVector Local = FMath::Lerp(De, Para, (I + 0.5f) / 3.f);
            USoundBase* Som = SonsPassos[FMath::RandRange(0, SonsPassos.Num() - 1)];
            TWeakObjectPtr<USoundBase> SomFraco(Som);
            TWeakObjectPtr<USoundAttenuation> AttFraca(AtenuacaoPassos);
            TWeakObjectPtr<ADiretorSustos> Eu2(this);
            AgendarLocal(Duracao * (I / 3.f), [Eu2, SomFraco, AttFraca, Local]()
            {
                if (Eu2.IsValid() && SomFraco.IsValid())
                {
                    UGameplayStatics::PlaySoundAtLocation(Eu2.Get(), SomFraco.Get(), Local, 1.6f, 1.25f, 0.f, AttFraca.Get());
                }
            });
        }
    }
}

// Variante: ela esta parada no fim do corredor, olhando para voce.
// A lampada mais perto dela estala e apaga... quando volta, nao tem mais ninguem.
bool ADiretorSustos::CriarVulto(APawn* Eu, const FVector& Olho, const FVector& Frente, float Profundidade, float Chao)
{
    const float Distancia = FMath::Clamp(Profundidade - 220.f, 1000.f, 1700.f);
    FVector Local = Olho + Frente * Distancia;
    Local.Z = Chao;

    FActorSpawnParameters SP;
    SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ASombraCorredor* Vulto = GetWorld()->SpawnActor<ASombraCorredor>(ClasseSombra ? ClasseSombra.Get() : ASombraCorredor::StaticClass(), Local, FRotator::ZeroRotator, SP);
    if (!Vulto) return false;

    const float Tempo = FMath::FRandRange(0.9f, 1.4f);
    Vulto->IniciarParado(Local, Eu->GetActorLocation(), Tempo + 0.05f);

    // A lampada mais proxima dele apaga no instante em que ele some
    const TArray<ALampada*> Perto = LampadasPerto(Local, 500.f);
    if (Perto.Num() > 0)
    {
        TWeakObjectPtr<ALampada> Fraca(Perto.Last());
        AgendarLocal(Tempo - 0.05f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(true, true); });
        AgendarLocal(Tempo + 0.25f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(false, true); });
    }

    // O som vem quando ele some, nao quando aparece (voce so percebe depois)
    if (USoundBase* Som = SomSombra.Get())
    {
        TWeakObjectPtr<USoundBase> SomFraco(Som);
        TWeakObjectPtr<ADiretorSustos> Eu2(this);
        const float Vol = VolumeSombra;
        AgendarLocal(Tempo - 0.05f, [Eu2, SomFraco, Vol]()
        {
            if (Eu2.IsValid() && SomFraco.IsValid()) UGameplayStatics::PlaySound2D(Eu2.Get(), SomFraco.Get(), Vol * 0.8f, 0.85f);
        });
    }

    UE_LOG(LogTemp, Log, TEXT("DiretorSustos: vulto parado a %.0f cm"), Distancia);
    return true;
}

// ======================================================================
// Batidas: alguem bate do OUTRO lado da parede, bem do seu lado
// ======================================================================
bool ADiretorSustos::AcharParedeAoLado(APawn* Alvo, FVector& Saida) const
{
    const FVector Base = Alvo->GetActorLocation();
    const FVector Frente = Alvo->GetBaseAimRotation().Vector().GetSafeNormal2D();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BatidasParede), false, Alvo);

    // Prefere paredes dos lados ou atras (fora da vista)
    float MelhorNota = -1.f;
    for (int32 I = 0; I < 12; ++I)
    {
        const FVector Dir = Frente.RotateAngleAxis(I * 30.f, FVector::UpVector);
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Base, Base + Dir * 450.f, ECC_Visibility, Params)) continue;
        if (FMath::Abs(Hit.ImpactNormal.Z) > 0.3f) continue;

        const float ForaDaVista = 1.f - FMath::Max(0.f, FVector::DotProduct(Dir, Frente));
        const float Perto = 1.f - Hit.Distance / 450.f;
        const float Nota = ForaDaVista * 0.7f + Perto * 0.3f;
        if (Nota > MelhorNota)
        {
            MelhorNota = Nota;
            Saida = Hit.ImpactPoint - Hit.ImpactNormal * 10.f;   // "dentro" da parede
        }
    }
    return MelhorNota >= 0.f;
}

void ADiretorSustos::Multicast_Batidas_Implementation(FVector_NetQuantize Local)
{
    if (USoundBase* Som = SomBatidas.Get())
    {
        UGameplayStatics::PlaySoundAtLocation(this, Som, Local, VolumeBatidas, FMath::FRandRange(0.92f, 1.03f), 0.f, AtenuacaoPassos);
    }

    // As lampadas em cima tremem com as batidas
    for (ALampada* L : LampadasPerto(Local, 300.f))
    {
        TWeakObjectPtr<ALampada> Fraca(L);
        AgendarLocal(0.05f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(true, false); });
        AgendarLocal(0.12f, [Fraca]() { if (Fraca.IsValid()) Fraca->ForcarApagado(false, false); });
    }
}
