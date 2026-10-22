#include "GeradorLevel0.h"
#include "Lampada.h"
#include "LampadaPainel.h"
#include "DiretorSustos.h"
#include "QuadroLuz.h"
#include "ParedeSaida.h"
#include "Bilhete.h"
#include "EnigmaLevel0.h"
#include "AmbienteLevel0.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace Level0
{
    const FName TagGerado(TEXT("GeradoPeloLevel0"));

    // Estado de cada parede entre duas celulas
    enum : uint8
    {
        Aberta = 0,
        Solida = 1,
        Vao    = 2   // parede com vao de porta
    };

    // Direcoes: 0 = +X, 1 = -X, 2 = +Y, 3 = -Y
    const int32 DX[4] = { 1, -1, 0, 0 };
    const int32 DY[4] = { 0, 0, 1, -1 };

    struct FSala
    {
        int32 X, Y, W, H;
    };
}

AGeradorLevel0::AGeradorLevel0()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Raiz = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));
    Raiz->SetMobility(EComponentMobility::Static);
    RootComponent = Raiz;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cubo.Succeeded())
    {
        MalhaCubo = Cubo.Object;
    }

    // Materiais: escolhidos no Blueprint (BP_GeradorLevel0) ou no Details do ator no mapa.
    // Classes: comecam com as classes C++; troque pelos BPs no BP_GeradorLevel0.
    ClasseLampada = ALampadaPainel::StaticClass();
    ClasseQuadro = AQuadroLuz::StaticClass();
    ClasseSaida = AParedeSaida::StaticClass();
    ClasseBilhete = ABilhete::StaticClass();
    ClasseEnigma = AEnigmaLevel0::StaticClass();
    ClasseAmbiente = AAmbienteLevel0::StaticClass();
    ClasseDiretor = ADiretorSustos::StaticClass();

    auto CriarInstancias = [this](const TCHAR* Nome, bool bColisao)
    {
        UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nome);
        C->SetupAttachment(RootComponent);
        C->SetMobility(EComponentMobility::Static);
        C->SetStaticMesh(MalhaCubo);
        if (bColisao)
        {
            C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        }
        else
        {
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        return C;
    };

    Paredes = CriarInstancias(TEXT("Paredes"), true);
    Rodapes = CriarInstancias(TEXT("Rodapes"), false);
    Pilares = CriarInstancias(TEXT("Pilares"), true);
    Tomadas = CriarInstancias(TEXT("Tomadas"), false);
    Grelhas = CriarInstancias(TEXT("Grelhas"), false);
    Tomadas->SetCastShadow(false);
    Grelhas->SetCastShadow(false);

    auto CriarLaje = [this](const TCHAR* Nome)
    {
        UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
        C->SetupAttachment(RootComponent);
        C->SetMobility(EComponentMobility::Static);
        C->SetStaticMesh(MalhaCubo);
        C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        return C;
    };

    Piso = CriarLaje(TEXT("Piso"));
    Teto = CriarLaje(TEXT("Teto"));
}

void AGeradorLevel0::Limpar()
{
    Paredes->Modify();
    Rodapes->Modify();
    Pilares->Modify();

    Tomadas->Modify();
    Grelhas->Modify();

    Paredes->ClearInstances();
    Rodapes->ClearInstances();
    Pilares->ClearInstances();
    Tomadas->ClearInstances();
    Grelhas->ClearInstances();

    for (AActor* Lampada : Lampadas)
    {
        if (IsValid(Lampada))
        {
            Lampada->Destroy();
        }
    }
    Lampadas.Empty();

    if (UWorld* Mundo = GetWorld())
    {
        for (TActorIterator<AActor> It(Mundo); It; ++It)
        {
            if (It->ActorHasTag(Level0::TagGerado))
            {
                It->Destroy();
            }
        }
    }
}

void AGeradorLevel0::NovaSemente()
{
    Semente = FMath::RandRange(1, 999999);
    Gerar();
}

void AGeradorLevel0::Gerar()
{
    using namespace Level0;

    Limpar();

    UWorld* Mundo = GetWorld();
    if (!Mundo || !MalhaCubo) return;

    FRandomStream Aleatorio(Semente);

    const int32 C = Colunas;
    const int32 L = Linhas;
    const float T = TamanhoCelula;
    const float H = AlturaTeto;
    const float E = EspessuraParede;

    // Paredes verticais: na borda X de cada celula (C+1 por linha)
    // Paredes horizontais: na borda Y de cada celula (L+1 por coluna)
    TArray<uint8> ParedeV;
    TArray<uint8> ParedeH;
    ParedeV.Init(Solida, (C + 1) * L);
    ParedeH.Init(Solida, C * (L + 1));

    auto IV = [C](int32 X, int32 Y) { return X + Y * (C + 1); };
    auto IH = [C](int32 X, int32 Y) { return X + Y * C; };
    auto Dentro = [C, L](int32 X, int32 Y) { return X >= 0 && Y >= 0 && X < C && Y < L; };

    // A parede entre a celula (X,Y) e a vizinha na direcao Dir
    auto ParedeEntre = [&](int32 X, int32 Y, int32 Dir) -> uint8&
    {
        switch (Dir)
        {
        case 0:  return ParedeV[IV(X + 1, Y)];
        case 1:  return ParedeV[IV(X, Y)];
        case 2:  return ParedeH[IH(X, Y + 1)];
        default: return ParedeH[IH(X, Y)];
        }
    };

    // ------------------------------------------------------------------
    // 1) Labirinto com preferencia por seguir reto (corredores longos)
    // ------------------------------------------------------------------
    {
        TArray<bool> Visitada;
        Visitada.Init(false, C * L);

        struct FPasso { int32 X, Y, Dir; };
        TArray<FPasso> Pilha;
        Pilha.Add({ 0, 0, -1 });
        Visitada[0] = true;

        while (Pilha.Num() > 0)
        {
            const FPasso Atual = Pilha.Last();

            int32 Opcoes[4];
            int32 NumOpcoes = 0;
            bool bRetoPossivel = false;
            for (int32 D = 0; D < 4; ++D)
            {
                const int32 NX = Atual.X + DX[D];
                const int32 NY = Atual.Y + DY[D];
                if (Dentro(NX, NY) && !Visitada[NX + NY * C])
                {
                    Opcoes[NumOpcoes++] = D;
                    bRetoPossivel |= (D == Atual.Dir);
                }
            }

            if (NumOpcoes == 0)
            {
                Pilha.Pop();
                continue;
            }

            const int32 Dir = (bRetoPossivel && Aleatorio.FRand() < ChanceSeguirReto)
                ? Atual.Dir
                : Opcoes[Aleatorio.RandRange(0, NumOpcoes - 1)];

            ParedeEntre(Atual.X, Atual.Y, Dir) = Aberta;

            const int32 NX = Atual.X + DX[Dir];
            const int32 NY = Atual.Y + DY[Dir];
            Visitada[NX + NY * C] = true;
            Pilha.Add({ NX, NY, Dir });
        }
    }

    // ------------------------------------------------------------------
    // 2) Salas: abre o interior de alguns retangulos
    // ------------------------------------------------------------------
    TArray<FSala> Salas;
    TArray<bool> EmSala;
    EmSala.Init(false, C * L);
    {
        const int32 MinS = FMath::Max(2, TamanhoSalaMin);
        const int32 MaxS = FMath::Max(MinS, TamanhoSalaMax);

        for (int32 I = 0; I < NumeroSalas; ++I)
        {
            const int32 W = FMath::Min(Aleatorio.RandRange(MinS, MaxS), C);
            const int32 Hs = FMath::Min(Aleatorio.RandRange(MinS, MaxS), L);
            const FSala S{ Aleatorio.RandRange(0, C - W), Aleatorio.RandRange(0, L - Hs), W, Hs };
            Salas.Add(S);

            for (int32 Y = S.Y; Y < S.Y + S.H; ++Y)
            {
                for (int32 X = S.X; X < S.X + S.W; ++X)
                {
                    EmSala[X + Y * C] = true;
                    if (X + 1 < S.X + S.W) ParedeV[IV(X + 1, Y)] = Aberta;
                    if (Y + 1 < S.Y + S.H) ParedeH[IH(X, Y + 1)] = Aberta;
                }
            }

            // Bordas da sala: algumas aberturas a mais para os corredores
            for (int32 X = S.X; X < S.X + S.W; ++X)
            {
                if (S.Y > 0 && Aleatorio.FRand() < 0.25f)       ParedeH[IH(X, S.Y)] = Aberta;
                if (S.Y + S.H < L && Aleatorio.FRand() < 0.25f) ParedeH[IH(X, S.Y + S.H)] = Aberta;
            }
            for (int32 Y = S.Y; Y < S.Y + S.H; ++Y)
            {
                if (S.X > 0 && Aleatorio.FRand() < 0.25f)       ParedeV[IV(S.X, Y)] = Aberta;
                if (S.X + S.W < C && Aleatorio.FRand() < 0.25f) ParedeV[IV(S.X + S.W, Y)] = Aberta;
            }
        }
    }

    // ------------------------------------------------------------------
    // 3) Paredes extras abertas (circuitos) e vaos de porta
    //    As bordas externas do mapa ficam sempre fechadas.
    // ------------------------------------------------------------------
    auto Variar = [&](uint8& Parede)
    {
        if (Parede != Solida) return;
        const float R = Aleatorio.FRand();
        if (R < ChanceAbrirExtra)                  Parede = Aberta;
        else if (R < ChanceAbrirExtra + ChanceVao) Parede = Vao;
    };
    for (int32 Y = 0; Y < L; ++Y)
    {
        for (int32 X = 1; X < C; ++X) Variar(ParedeV[IV(X, Y)]);
    }
    for (int32 Y = 1; Y < L; ++Y)
    {
        for (int32 X = 0; X < C; ++X) Variar(ParedeH[IH(X, Y)]);
    }

    // ------------------------------------------------------------------
    // 4) Zona escura: longe do inicio, escurece ate o centro
    // ------------------------------------------------------------------
    TArray<float> FatorLuz;
    FatorLuz.Init(1.f, C * L);
    for (int32 Z = 0; Z < NumeroZonasEscuras; ++Z)
    {
        int32 CX = 0, CY = 0;
        for (int32 Tentativa = 0; Tentativa < 20; ++Tentativa)
        {
            CX = Aleatorio.RandRange(0, C - 1);
            CY = Aleatorio.RandRange(0, L - 1);
            if (FMath::Abs(CX) + FMath::Abs(CY) > 8) break;   // nunca perto do inicio
        }
        for (int32 Y = 0; Y < L; ++Y)
        {
            for (int32 X = 0; X < C; ++X)
            {
                const float Dist = FVector2D::Distance(FVector2D(X, Y), FVector2D(CX, CY));
                FatorLuz[X + Y * C] = FMath::Min(FatorLuz[X + Y * C], FMath::Clamp(Dist / RaioZonaEscura, 0.f, 1.f));
            }
        }
    }

    // ------------------------------------------------------------------
    // 5) Geometria
    // ------------------------------------------------------------------
    Paredes->SetMaterial(0, MaterialParede);
    Pilares->SetMaterial(0, MaterialParede);
    Rodapes->SetMaterial(0, MaterialRodape ? MaterialRodape.Get() : MaterialParede.Get());
    Tomadas->SetMaterial(0, MaterialTomada ? MaterialTomada.Get() : MaterialParede.Get());
    Grelhas->SetMaterial(0, MaterialGrelha ? MaterialGrelha.Get() : MaterialParede.Get());

    auto Caixa = [](UInstancedStaticMeshComponent* Comp, const FVector& Centro, const FVector& TamanhoCm)
    {
        Comp->AddInstance(FTransform(FRotator::ZeroRotator, Centro, TamanhoCm / 100.f), false);
    };

    const float LarguraRodape = E + EspessuraRodape * 2.f;

    // Um trecho de parede (com rodape dos dois lados) ao longo de X ou de Y
    auto Trecho = [&](const FVector& CentroBase, bool bAoLongoY, float Comprimento, float ZBase, float Altura, bool bRodape)
    {
        if (Comprimento <= 1.f || Altura <= 1.f) return;
        const FVector Tam = bAoLongoY ? FVector(E, Comprimento, Altura) : FVector(Comprimento, E, Altura);
        Caixa(Paredes, CentroBase + FVector(0.f, 0.f, ZBase + Altura * 0.5f), Tam);

        if (bRodape)
        {
            const FVector TamR = bAoLongoY ? FVector(LarguraRodape, Comprimento, AlturaRodape)
                                           : FVector(Comprimento, LarguraRodape, AlturaRodape);
            Caixa(Rodapes, CentroBase + FVector(0.f, 0.f, AlturaRodape * 0.5f), TamR);
        }
    };

    auto ConstruirParede = [&](const FVector& Centro, bool bAoLongoY, uint8 Tipo)
    {
        const float Comp = T + E;   // passa um pouco para fechar os cantos
        if (Tipo == Solida)
        {
            Trecho(Centro, bAoLongoY, Comp, 0.f, H, true);

            // De vez em quando, uma tomada velha perto do rodape
            if (Aleatorio.FRand() < ChanceTomada)
            {
                const FVector Normal = bAoLongoY ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 1.f, 0.f);
                const FVector Eixo = bAoLongoY ? FVector(0.f, 1.f, 0.f) : FVector(1.f, 0.f, 0.f);
                const float Lado = Aleatorio.RandBool() ? 1.f : -1.f;
                const FVector Pos = Centro + Normal * Lado * (E * 0.5f + 0.6f)
                    + Eixo * Aleatorio.FRandRange(-(T * 0.5f - 50.f), T * 0.5f - 50.f)
                    + FVector(0.f, 0.f, 32.f);
                const FVector Tam = bAoLongoY ? FVector(1.2f, 8.f, 12.f) : FVector(8.f, 1.2f, 12.f);
                Caixa(Tomadas, Pos, Tam);
            }
            return;
        }
        if (Tipo != Vao) return;

        const FVector Eixo = bAoLongoY ? FVector(0.f, 1.f, 0.f) : FVector(1.f, 0.f, 0.f);
        const float Largura = FMath::Min(LarguraVao, T - 60.f);
        const float Folga = FMath::Max(0.f, (T - Largura) * 0.5f - 30.f);
        const float Desloc = Aleatorio.FRandRange(-Folga, Folga);

        const float Inicio = -Comp * 0.5f;
        const float Fim = Comp * 0.5f;
        const float VaoIni = Desloc - Largura * 0.5f;
        const float VaoFim = Desloc + Largura * 0.5f;

        // Lado esquerdo e direito do vao
        Trecho(Centro + Eixo * ((Inicio + VaoIni) * 0.5f), bAoLongoY, VaoIni - Inicio, 0.f, H, true);
        Trecho(Centro + Eixo * ((VaoFim + Fim) * 0.5f), bAoLongoY, Fim - VaoFim, 0.f, H, true);

        // Verga (parede acima do vao)
        Trecho(Centro + Eixo * Desloc, bAoLongoY, Largura, AlturaVao, H - AlturaVao, false);
    };

    for (int32 Y = 0; Y < L; ++Y)
    {
        for (int32 X = 0; X <= C; ++X)
        {
            ConstruirParede(FVector((X - 0.5f) * T, Y * T, 0.f), true, ParedeV[IV(X, Y)]);
        }
    }
    for (int32 Y = 0; Y <= L; ++Y)
    {
        for (int32 X = 0; X < C; ++X)
        {
            ConstruirParede(FVector(X * T, (Y - 0.5f) * T, 0.f), false, ParedeH[IH(X, Y)]);
        }
    }

    // Colunas soltas dentro das salas (nos cantos internos entre celulas)
    for (const FSala& S : Salas)
    {
        for (int32 Y = S.Y + 1; Y < S.Y + S.H; ++Y)
        {
            for (int32 X = S.X + 1; X < S.X + S.W; ++X)
            {
                if (Aleatorio.FRand() >= ChancePilar) continue;
                const FVector Centro((X - 0.5f) * T, (Y - 0.5f) * T, 0.f);
                Caixa(Pilares, Centro + FVector(0.f, 0.f, H * 0.5f), FVector(LadoPilar, LadoPilar, H));
                Caixa(Rodapes, Centro + FVector(0.f, 0.f, AlturaRodape * 0.5f),
                    FVector(LadoPilar + EspessuraRodape * 2.f, LadoPilar + EspessuraRodape * 2.f, AlturaRodape));
            }
        }
    }

    // Grelhas de ventilacao no forro (longe do centro, onde fica a lampada)
    for (int32 Y = 0; Y < L; ++Y)
    {
        for (int32 X = 0; X < C; ++X)
        {
            if (Aleatorio.FRand() >= ChanceGrelha) continue;
            const float SX = Aleatorio.RandBool() ? 1.f : -1.f;
            const float SY = Aleatorio.RandBool() ? 1.f : -1.f;
            // Ocupa uma placa inteira do forro (placas de 60 cm, alinhadas com as celulas)
            const FVector Pos(X * T + SX * 90.f, Y * T + SY * 90.f, H - 0.75f);
            Caixa(Grelhas, Pos, FVector(56.f, 56.f, 1.5f));
        }
    }

    // Piso e teto: lajes unicas (texturas alinhadas ao mundo, sem emendas)
    const FVector CentroMapa((C - 1) * T * 0.5f, (L - 1) * T * 0.5f, 0.f);
    const FVector TamanhoLaje(C * T + E, L * T + E, E);

    Piso->Modify();
    Piso->SetRelativeLocation(CentroMapa + FVector(0.f, 0.f, -E * 0.5f));
    Piso->SetRelativeScale3D(TamanhoLaje / 100.f);
    Piso->SetMaterial(0, MaterialPiso);

    Teto->Modify();
    Teto->SetRelativeLocation(CentroMapa + FVector(0.f, 0.f, H + E * 0.5f));
    Teto->SetRelativeScale3D(TamanhoLaje / 100.f);
    Teto->SetMaterial(0, MaterialTeto);

    // ------------------------------------------------------------------
    // 5b) Enigma de saida: escolhe as paredes dos quadros, da saida e dos bilhetes
    // ------------------------------------------------------------------
    struct FSlot { int32 X, Y, Dir; };
    TArray<FSlot> Slots;
    for (int32 Y = 0; Y < L; ++Y)
    {
        for (int32 X = 0; X < C; ++X)
        {
            for (int32 D = 0; D < 4; ++D)
            {
                if (ParedeEntre(X, Y, D) == Solida) Slots.Add({ X, Y, D });
            }
        }
    }

    auto Dist = [](int32 AX, int32 AY, int32 BX, int32 BY)
    {
        return FMath::Sqrt(float((AX - BX) * (AX - BX) + (AY - BY) * (AY - BY)));
    };

    TSet<int32> CelulasUsadas;
    auto Escolher = [&](TFunctionRef<bool(const FSlot&)> Filtro, FSlot& Saida) -> bool
    {
        if (Slots.Num() == 0) return false;
        for (int32 Tentativa = 0; Tentativa < 600; ++Tentativa)
        {
            const FSlot& S = Slots[Aleatorio.RandRange(0, Slots.Num() - 1)];
            if (CelulasUsadas.Contains(S.X + S.Y * C)) continue;
            if (!Filtro(S)) continue;
            Saida = S;
            CelulasUsadas.Add(S.X + S.Y * C);
            return true;
        }
        return false;
    };
    auto LongeDe = [&](const FSlot& S, const TArray<FSlot>& Outros, float Minimo)
    {
        for (const FSlot& O : Outros)
        {
            if (Dist(S.X, S.Y, O.X, O.Y) < Minimo) return false;
        }
        return true;
    };

    // Grupo 0 (1-2 jogadores): tres quadros na mesma regiao, longe do inicio
    TArray<FSlot> QuadrosPerto;
    {
        const int32 CX = FMath::Clamp(Aleatorio.RandRange(C / 3, (C * 2) / 3), 0, C - 1);
        const int32 CY = FMath::Clamp(Aleatorio.RandRange(L / 3, (L * 2) / 3), 0, L - 1);
        for (int32 I = 0; I < 3; ++I)
        {
            FSlot S{ -1, -1, 0 };
            if (Escolher([&](const FSlot& A)
                {
                    return Dist(A.X, A.Y, CX, CY) <= 5.f && Dist(A.X, A.Y, 0, 0) >= 5.f && LongeDe(A, QuadrosPerto, 2.5f);
                }, S))
            {
                QuadrosPerto.Add(S);
            }
        }
    }

    // Grupo 1 (3-5 jogadores): um quadro em cada canto do mapa (menos o do inicio)
    TArray<FSlot> QuadrosLonge;
    {
        const int32 MX = C / 2, MY = L / 2;
        const FIntRect Regioes[3] = {
            FIntRect(MX, 0, C, MY),
            FIntRect(0, MY, MX, L),
            FIntRect(MX, MY, C, L)
        };
        for (const FIntRect& R : Regioes)
        {
            FSlot S{ -1, -1, 0 };
            if (Escolher([&](const FSlot& A)
                {
                    return A.X >= R.Min.X && A.X < R.Max.X && A.Y >= R.Min.Y && A.Y < R.Max.Y && LongeDe(A, QuadrosLonge, 6.f);
                }, S))
            {
                QuadrosLonge.Add(S);
            }
        }
    }

    TArray<FSlot> TodosQuadros = QuadrosPerto;
    TodosQuadros.Append(QuadrosLonge);

    // Saida: a parede mais longe do inicio entre varias sorteadas
    FSlot SlotSaida{ -1, -1, 0 };
    {
        float MelhorDist = -1.f;
        for (int32 I = 0; I < 80 && Slots.Num() > 0; ++I)
        {
            const FSlot& S = Slots[Aleatorio.RandRange(0, Slots.Num() - 1)];
            if (CelulasUsadas.Contains(S.X + S.Y * C) || !LongeDe(S, TodosQuadros, 4.f)) continue;
            const float D = Dist(S.X, S.Y, 0, 0);
            if (D > MelhorDist) { MelhorDist = D; SlotSaida = S; }
        }
        if (SlotSaida.X >= 0) CelulasUsadas.Add(SlotSaida.X + SlotSaida.Y * C);
    }

    // Bilhetes: o primeiro sempre perto do inicio
    const TCHAR* TextosBilhetes[] = {
        TEXT("As luzes nunca\ndesligam.\nEu achei onde\nelas MORAM.\nSiga o fio no teto."),
        TEXT("Onde a l\u00E2mpada\nn\u00E3o canta,\na caixa respira."),
        TEXT("Tr\u00EAs caixas.\nDesliga uma e as\noutras voltam.\nTem que ser r\u00E1pido.\nEu estava sozinho."),
        TEXT("No escuro tem\numa placa verde.\nNo claro, ela\nnunca existiu."),
        TEXT("Eu cheguei.\n\u00C9 s\u00F3 parede.\nEla sabe quando\neu olho."),
        TEXT("Os passos param\nquando voc\u00EA vira.\nA parede tamb\u00E9m.\nEntre como quem\nfoge: sem olhar.")
    };
    // Alguns bilhetes mudam depois que voce le e vai embora
    const TCHAR* TextosDepois[] = {
        TEXT("Voc\u00EA j\u00E1 leu isso.\nPor que voltou?"),
        TEXT(""),
        TEXT(""),
        TEXT(""),
        TEXT("Ela viu\nvoc\u00EA ler isso."),
        TEXT("")
    };
    TArray<FSlot> SlotsBilhetes;
    TArray<FSlot> Ocupados = TodosQuadros;
    if (SlotSaida.X >= 0) Ocupados.Add(SlotSaida);
    for (int32 I = 0; I < UE_ARRAY_COUNT(TextosBilhetes); ++I)
    {
        FSlot S{ -1, -1, 0 };
        const bool bOk = (I == 0)
            ? Escolher([&](const FSlot& A) { const float D = Dist(A.X, A.Y, 0, 0); return D >= 1.f && D <= 4.f; }, S)
            : Escolher([&](const FSlot& A) { return LongeDe(A, SlotsBilhetes, 4.f) && LongeDe(A, Ocupados, 2.f); }, S);
        SlotsBilhetes.Add(bOk ? S : FSlot{ -1, -1, 0 });
    }

    // Celulas onde a lampada precisa existir (a lampada sem zumbido acima do quadro)
    TSet<int32> LampadaObrigatoria;
    for (const FSlot& S : TodosQuadros) LampadaObrigatoria.Add(S.X + S.Y * C);
    TMap<int32, ALampada*> LampadaPorCelula;

    // ------------------------------------------------------------------
    // 6) Lampadas: uma por celula, alinhada com o corredor
    // ------------------------------------------------------------------
    if (ClasseLampada)
    {
        for (int32 Y = 0; Y < L; ++Y)
        {
            for (int32 X = 0; X < C; ++X)
            {
                const float Fator = FatorLuz[X + Y * C];
                const bool bInicio = (X == 0 && Y == 0) || LampadaObrigatoria.Contains(X + Y * C);
                const bool bEscuro = Fator < 0.6f;

                const bool bAcesa = bInicio || Aleatorio.FRand() < ChanceLampada * Fator;
                const bool bQueimada = !bAcesa && (bEscuro ? Aleatorio.FRand() < 0.6f
                                                           : Aleatorio.FRand() < ChanceLampadaQueimada / FMath::Max(0.01f, 1.f - ChanceLampada));
                if (!bAcesa && !bQueimada) continue;

                const bool bLuzReal = bInicio || Aleatorio.FRand() < FracaoLuzReal;
                const bool bPiscando = bAcesa && !bInicio && (bEscuro || Aleatorio.FRand() < ChanceLampadaPiscando);

                // Corredor ao longo de Y (paredes dos lados X) -> gira a lampada
                const bool bFechadoX = ParedeEntre(X, Y, 0) != Aberta && ParedeEntre(X, Y, 1) != Aberta;
                const bool bAbertoY = ParedeEntre(X, Y, 2) == Aberta || ParedeEntre(X, Y, 3) == Aberta;
                const float Yaw = (bFechadoX && bAbertoY && !EmSala[X + Y * C]) ? 90.f : 0.f;

                // Lado curto da lampada: Y quando ela esta em 0 graus, X quando esta em 90
                const FVector Desloc = (Yaw == 0.f) ? FVector(0.f, DeslocamentoLampada, 0.f) : FVector(DeslocamentoLampada, 0.f, 0.f);
                const FVector Local = GetActorTransform().TransformPosition(FVector(X * T, Y * T, H - 1.f) + Desloc);
                const FTransform Transform(FRotator(0.f, Yaw, 0.f), Local);

                ALampada* Lampada = Mundo->SpawnActorDeferred<ALampada>(ClasseLampada, Transform, this);
                if (!Lampada) continue;

                if (bQueimada)
                {
                    Lampada->ConfigurarEstado(true, 0.f, true);
                }
                else
                {
                    Lampada->ConfigurarEstado(false, bPiscando ? ChanceRajadaPiscando : 0.f, !bLuzReal);
                }

                Lampada->Tags.Add(TagGerado);
                Lampada->FinishSpawning(Transform);
#if WITH_EDITOR
                Lampada->SetFolderPath(TEXT("Level0/Lampadas"));
#endif
                Lampadas.Add(Lampada);
                LampadaPorCelula.Add(X + Y * C, Lampada);
            }
        }
    }

    // ------------------------------------------------------------------
    // 6b) Enigma: cria quadros, saida, bilhetes e o controlador
    // ------------------------------------------------------------------
    {
        // Ator encostado na parede, olhando para dentro da celula
        auto TransformParede = [&](const FSlot& S)
        {
            const FVector DirV(DX[S.Dir], DY[S.Dir], 0.f);
            const FVector LocalPos = FVector(S.X * T, S.Y * T, 0.f) + DirV * (T * 0.5f - E * 0.5f);
            return FTransform((-DirV).Rotation(), LocalPos) * GetActorTransform();
        };

        auto Finalizar = [&](AActor* A, const FTransform& Tr)
        {
            A->Tags.Add(TagGerado);
            A->FinishSpawning(Tr);
#if WITH_EDITOR
            A->SetFolderPath(TEXT("Level0/Enigma"));
#endif
        };

        const TCHAR* Contagem[] = { TEXT("I"), TEXT("II"), TEXT("III") };
        auto CriarQuadros = [&](const TArray<FSlot>& Lista, int32 Grupo)
        {
            for (int32 I = 0; I < Lista.Num(); ++I)
            {
                const FTransform Tr = TransformParede(Lista[I]);
                AQuadroLuz* Q = Mundo->SpawnActorDeferred<AQuadroLuz>(ClasseQuadro ? ClasseQuadro.Get() : AQuadroLuz::StaticClass(), Tr, this);
                if (!Q) continue;
                Q->Grupo = Grupo;
                Q->TextoRabisco = FText::FromString(Contagem[FMath::Min(I, 2)]);
                Q->AlturaTeto = H;
                Q->ComprimentoConduite = Aleatorio.FRandRange(150.f, 450.f);
                if (ALampada** Achou = LampadaPorCelula.Find(Lista[I].X + Lista[I].Y * C))
                {
                    Q->LampadaSilenciosa = *Achou;
                }
                Finalizar(Q, Tr);
            }
        };
        CriarQuadros(QuadrosPerto, 0);
        CriarQuadros(QuadrosLonge, 1);

        if (SlotSaida.X >= 0)
        {
            const FTransform Tr = TransformParede(SlotSaida);
            if (AParedeSaida* P = Mundo->SpawnActorDeferred<AParedeSaida>(ClasseSaida ? ClasseSaida.Get() : AParedeSaida::StaticClass(), Tr, this))
            {
                Finalizar(P, Tr);
            }
        }

        for (int32 I = 0; I < SlotsBilhetes.Num(); ++I)
        {
            if (SlotsBilhetes[I].X < 0) continue;
            // Cada folha num ponto diferente da parede
            FTransform Tr = TransformParede(SlotsBilhetes[I]);
            Tr.AddToTranslation(Tr.GetRotation().GetRightVector() * Aleatorio.FRandRange(-90.f, 90.f));
            if (ABilhete* B = Mundo->SpawnActorDeferred<ABilhete>(ClasseBilhete ? ClasseBilhete.Get() : ABilhete::StaticClass(), Tr, this))
            {
                B->Texto = FText::FromString(TextosBilhetes[I]);
                if (I < UE_ARRAY_COUNT(TextosDepois))
                {
                    B->TextoDepois = FText::FromString(TextosDepois[I]);
                }
                Finalizar(B, Tr);
            }
        }

        const FTransform TrEnigma = GetActorTransform();
        if (AEnigmaLevel0* Enigma = Mundo->SpawnActorDeferred<AEnigmaLevel0>(ClasseEnigma ? ClasseEnigma.Get() : AEnigmaLevel0::StaticClass(), TrEnigma, this))
        {
            // Rota de fuga: busca em largura a partir da celula da saida
            TArray<int32> Proxima;
            Proxima.Init(-2, C * L);   // -2 = sem caminho
            if (SlotSaida.X >= 0)
            {
                const int32 Saida = SlotSaida.X + SlotSaida.Y * C;
                TArray<int32> Fila;
                Fila.Add(Saida);
                Proxima[Saida] = -1;
                for (int32 I = 0; I < Fila.Num(); ++I)
                {
                    const int32 Atual = Fila[I];
                    const int32 AX = Atual % C;
                    const int32 AY = Atual / C;
                    for (int32 D = 0; D < 4; ++D)
                    {
                        const int32 NX = AX + DX[D];
                        const int32 NY = AY + DY[D];
                        if (!Dentro(NX, NY) || ParedeEntre(AX, AY, D) == Solida) continue;
                        const int32 Vizinha = NX + NY * C;
                        if (Proxima[Vizinha] != -2) continue;
                        Proxima[Vizinha] = Atual;   // da vizinha, o proximo passo e voltar para "Atual"
                        Fila.Add(Vizinha);
                    }
                }
            }
            Enigma->CaminhoSaida = Proxima;
            Enigma->MapaColunas = C;
            Enigma->MapaLinhas = L;
            Enigma->MapaCelula = T;
            Enigma->MapaAltura = H;
            Enigma->MapaOrigem = GetActorTransform();

            Finalizar(Enigma, TrEnigma);
        }

        UE_LOG(LogTemp, Log, TEXT("GeradorLevel0: enigma com %d+%d quadros, saida %s, %d bilhetes."),
            QuadrosPerto.Num(), QuadrosLonge.Num(), SlotSaida.X >= 0 ? TEXT("ok") : TEXT("FALTOU"), SlotsBilhetes.Num());
    }

    // ------------------------------------------------------------------
    // 7) PlayerStart na primeira celula, olhando para um lado aberto
    // ------------------------------------------------------------------
    float YawInicio = 0.f;
    if (ParedeEntre(0, 0, 0) != Aberta && ParedeEntre(0, 0, 2) == Aberta)
    {
        YawInicio = 90.f;
    }
    for (TActorIterator<APlayerStart> It(Mundo); It; ++It)
    {
        It->SetActorLocationAndRotation(GetActorTransform().TransformPosition(FVector(0.f, 0.f, 100.f)), FRotator(0.f, YawInicio, 0.f));
        break;
    }

    // ------------------------------------------------------------------
    // 7b) Som de fundo e intro
    // ------------------------------------------------------------------
    {
        const FTransform TrAmb = GetActorTransform();
        if (AAmbienteLevel0* Amb = Mundo->SpawnActorDeferred<AAmbienteLevel0>(ClasseAmbiente ? ClasseAmbiente.Get() : AAmbienteLevel0::StaticClass(), TrAmb, this))
        {
            Amb->Tags.Add(TagGerado);
            Amb->FinishSpawning(TrAmb);
#if WITH_EDITOR
            Amb->SetFolderPath(TEXT("Level0"));
#endif
        }
    }

    // ------------------------------------------------------------------
    // 8) Diretor de Sustos
    // ------------------------------------------------------------------
    if (bCriarDiretorSustos)
    {
        bool bExiste = false;
        for (TActorIterator<ADiretorSustos> It(Mundo); It; ++It)
        {
            bExiste = true;
            break;
        }
        if (!bExiste)
        {
            if (ADiretorSustos* Diretor = Mundo->SpawnActor<ADiretorSustos>(ClasseDiretor ? ClasseDiretor.Get() : ADiretorSustos::StaticClass(), GetActorLocation(), FRotator::ZeroRotator))
            {
#if WITH_EDITOR
                Diretor->SetFolderPath(TEXT("Level0"));
#endif
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GeradorLevel0: %d paredes, %d lampadas, %d salas (semente %d)."),
        Paredes->GetInstanceCount(), Lampadas.Num(), Salas.Num(), Semente);
}
