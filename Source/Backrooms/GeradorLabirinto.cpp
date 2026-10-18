#include "GeradorLabirinto.h"
#include "Celula.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float TamanhoCelula = 400.f;   // igual ao tamanho da Celula

    // Informações de uma célula durante a geração (antes de criar no mundo)
    struct FDadosCelula
    {
        bool Norte = true;
        bool Sul = true;
        bool Leste = true;
        bool Oeste = true;
        bool bVisitada = false;
    };
}

AGeradorLabirinto::AGeradorLabirinto()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    // Pilar padrão: o cubo da engine (dá para trocar no painel Details)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cubo.Succeeded())
    {
        MalhaPilar = Cubo.Object;
    }
}

void AGeradorLabirinto::Limpar()
{
    for (ACelula* Celula : Celulas)
    {
        if (IsValid(Celula))
        {
            Celula->Destroy();
        }
    }
    Celulas.Empty();

    for (AActor* Pilar : Pilares)
    {
        if (IsValid(Pilar))
        {
            Pilar->Destroy();
        }
    }
    Pilares.Empty();
}

void AGeradorLabirinto::NovaSemente()
{
    Semente = FMath::RandRange(1, 999999);
    Gerar();
}

void AGeradorLabirinto::Gerar()
{
    Limpar();

    UWorld* Mundo = GetWorld();

    if (!Mundo || !ClasseCelula)
    {

        UE_LOG(LogTemp, Warning, TEXT("GeradorLabirinto: escolha a Classe Celula no painel Detalhes."));
        return;
    }

    // Gerador de números aleatórios com semente: sempre a mesma sequência para a mesma semente
    FRandomStream Aleatorio(Semente);

    TArray<FDadosCelula> Grade;
    Grade.SetNum(Colunas * Linhas);

    auto Indice = [this](int32 X, int32 Y) { return Y * Colunas + X; };
    auto Dentro = [this](int32 X, int32 Y) { return X >= 0 && X < Colunas && Y >= 0 && Y < Linhas; };

    // Abre a passagem entre duas vizinhas: tira AS DUAS paredes da divisa
    auto Abrir = [&](int32 X, int32 Y, int32 DX, int32 DY)
        {
            FDadosCelula& A = Grade[Indice(X, Y)];
            FDadosCelula& B = Grade[Indice(X + DX, Y + DY)];
            if (DX == 1) { A.Norte = false; B.Sul = false; }
            if (DX == -1) { A.Sul = false; B.Norte = false; }
            if (DY == 1) { A.Leste = false; B.Oeste = false; }
            if (DY == -1) { A.Oeste = false; B.Leste = false; }
        };

    // ---------- 1) Labirinto base (busca em profundidade) ----------
    // Anda de célula em célula abrindo caminho; quando trava, volta e tenta outro lado.
    const FIntPoint Direcoes[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

    TArray<FIntPoint> Pilha;
    Pilha.Push(FIntPoint(0, 0));
    Grade[Indice(0, 0)].bVisitada = true;

    while (Pilha.Num() > 0)
    {
        const FIntPoint Atual = Pilha.Last();

        // Vizinhas que ainda não foram visitadas
        TArray<FIntPoint, TInlineAllocator<4>> Opcoes;
        for (const FIntPoint& D : Direcoes)
        {
            const int32 NX = Atual.X + D.X;
            const int32 NY = Atual.Y + D.Y;
            if (Dentro(NX, NY) && !Grade[Indice(NX, NY)].bVisitada)
            {
                Opcoes.Add(D);
            }
        }

        if (Opcoes.Num() == 0)
        {
            Pilha.Pop();   // beco sem saída: volta um passo
            continue;
        }

        const FIntPoint D = Opcoes[Aleatorio.RandRange(0, Opcoes.Num() - 1)];
        Abrir(Atual.X, Atual.Y, D.X, D.Y);

        const FIntPoint Proxima(Atual.X + D.X, Atual.Y + D.Y);
        Grade[Indice(Proxima.X, Proxima.Y)].bVisitada = true;
        Pilha.Push(Proxima);
    }

    // ---------- 2) Aberturas extras (voltas, atalhos, áreas abertas) ----------
    for (int32 Y = 0; Y < Linhas; ++Y)
    {
        for (int32 X = 0; X < Colunas; ++X)
        {
            if (X + 1 < Colunas && Aleatorio.FRand() < ChanceAberturaExtra) Abrir(X, Y, 1, 0);
            if (Y + 1 < Linhas && Aleatorio.FRand() < ChanceAberturaExtra) Abrir(X, Y, 0, 1);
        }
    }

    // ---------- 2b) Salões abertos ----------
    for (int32 S = 0; S < NumeroDeSaloes; ++S)
    {
        const int32 Largura = FMath::Min(Aleatorio.RandRange(TamanhoSalaoMin, TamanhoSalaoMax), Colunas);
        const int32 Altura = FMath::Min(Aleatorio.RandRange(TamanhoSalaoMin, TamanhoSalaoMax), Linhas);
        const int32 X0 = Aleatorio.RandRange(0, Colunas - Largura);
        const int32 Y0 = Aleatorio.RandRange(0, Linhas - Altura);

        // Remove todas as paredes internas do retângulo
        for (int32 Y = Y0; Y < Y0 + Altura; ++Y)
        {
            for (int32 X = X0; X < X0 + Largura; ++X)
            {
                if (X + 1 < X0 + Largura) Abrir(X, Y, 1, 0);
                if (Y + 1 < Y0 + Altura)  Abrir(X, Y, 0, 1);
            }
        }
    }

    // ---------- 3) Cria as células no mundo ----------
    const FVector Origem = GetActorLocation();

    for (int32 Y = 0; Y < Linhas; ++Y)
    {
        for (int32 X = 0; X < Colunas; ++X)
        {
            const FDadosCelula& Dados = Grade[Indice(X, Y)];
            const FTransform Transform(FRotator::ZeroRotator,
                Origem + FVector(X * TamanhoCelula, Y * TamanhoCelula, 0.f));

            // "Deferred" = cria, configura as paredes e só depois termina de montar
            ACelula* Celula = Mundo->SpawnActorDeferred<ACelula>(ClasseCelula, Transform, this);
            if (!Celula) continue;

            Celula->bParedeNorte = Dados.Norte;
            Celula->bParedeSul = Dados.Sul;
            Celula->bParedeLeste = Dados.Leste;
            Celula->bParedeOeste = Dados.Oeste;
            Celula->bTemLampada = Aleatorio.FRand() < ChanceLampada;

            Celula->FinishSpawning(Transform);

#if WITH_EDITOR
            Celula->SetFolderPath(TEXT("Labirinto"));   // organiza no Organizador
#endif
            Celulas.Add(Celula);
        }
    }

    // ---------- 3b) Pilares nos cantos abertos ----------
// Um canto fica entre 4 células. Se nenhuma parede encosta nele, pode ter um pilar.
    if (MalhaPilar)
    {
        for (int32 Y = 1; Y < Linhas; ++Y)
        {
            for (int32 X = 1; X < Colunas; ++X)
            {
                const FDadosCelula& SudOeste = Grade[Indice(X - 1, Y - 1)];
                const FDadosCelula& NorOeste = Grade[Indice(X, Y - 1)];
                const FDadosCelula& SulLeste = Grade[Indice(X - 1, Y)];

                const bool bCantoAberto = !SudOeste.Norte && !SulLeste.Norte
                    && !SudOeste.Leste && !NorOeste.Leste;

                if (!bCantoAberto || Aleatorio.FRand() >= ChancePilar) continue;

                const FVector Local = Origem + FVector((X - 0.5f) * TamanhoCelula,
                    (Y - 0.5f) * TamanhoCelula,
                    150.f);   // metade da altura (3 m)

                AStaticMeshActor* Pilar = Mundo->SpawnActor<AStaticMeshActor>(Local, FRotator::ZeroRotator);
                if (!Pilar) continue;

                UStaticMeshComponent* Malha = Pilar->GetStaticMeshComponent();
                Malha->SetStaticMesh(MalhaPilar);
                Malha->SetWorldScale3D(FVector(0.6f, 0.6f, 3.f));   // 60 x 60 cm, 3 m de altura
                if (MaterialPilar)
                {
                    Malha->SetMaterial(0, MaterialPilar);
                }
#if WITH_EDITOR
                Pilar->SetFolderPath(TEXT("Labirinto/Pilares"));
                Pilar->SetActorLabel(FString::Printf(TEXT("Pilar_%d_%d"), X, Y));
#endif
                Pilares.Add(Pilar);
            }
        }
    }

    const FDadosCelula& Inicio = Grade[Indice(0, 0)];
    float Yaw = 0.f;                                   // Norte (+X)
    if (!Inicio.Norte) Yaw = 0.f;
    else if (!Inicio.Leste) Yaw = 90.f;                // Leste (+Y)
    else if (!Inicio.Sul)   Yaw = 180.f;               // Sul (-X)
    else if (!Inicio.Oeste) Yaw = -90.f;               // Oeste (-Y)

    for (TActorIterator<APlayerStart> It(Mundo); It; ++It)
    {
        It->SetActorLocationAndRotation(Origem + FVector(0.f, 0.f, 100.f), FRotator(0.f, Yaw, 0.f));
        break;   // usa só o primeiro PlayerStart
    }

    UE_LOG(LogTemp, Log, TEXT("GeradorLabirinto: %d células geradas (semente %d)."), Celulas.Num(), Semente);
}