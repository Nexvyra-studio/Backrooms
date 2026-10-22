#include "GeradorLabirinto.h"
#include "Lampada.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float TamanhoCelula = 400.f;   // 4 m
    constexpr float Meio          = TamanhoCelula * 0.5f;
    constexpr float AlturaTeto    = 300.f;   // 3 m de pe-direito
    constexpr float Espessura     = 20.f;    // paredes, piso e teto
    constexpr float SobraMinima   = 30.f;    // parede minima ao lado de um vao

    const FName TagGerado(TEXT("GeradoPeloLabirinto"));

    enum class ETipoAresta : uint8
    {
        Aberta,      // sem parede
        Solida,      // parede inteira
        Porta,       // vao estreito com parede em cima
        Passagem,    // vao largo com parede em cima
        PortaFalsa   // vao emparedado (porta para lugar nenhum)
    };

    // Uma "aresta" e o trecho de 4 m entre duas celulas vizinhas
    struct FAresta
    {
        ETipoAresta Tipo = ETipoAresta::Aberta;
        float Deslocamento = 0.f;   // posicao do vao ao longo da parede (0 = centro)
    };

    struct FRegiao
    {
        int32 X, Y, W, H;
    };
}

AGeradorLabirinto::AGeradorLabirinto()
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

void AGeradorLabirinto::Limpar()
{
    Paredes->Modify();
    Rodapes->Modify();
    Pilares->Modify();

    Paredes->ClearInstances();
    Rodapes->ClearInstances();
    Pilares->ClearInstances();

    for (AActor* Lampada : Lampadas)
    {
        if (IsValid(Lampada))
        {
            Lampada->Destroy();
        }
    }
    Lampadas.Empty();

    // Garante que nada gerado antes ficou para tras
    if (UWorld* Mundo = GetWorld())
    {
        for (TActorIterator<AActor> It(Mundo); It; ++It)
        {
            if (It->ActorHasTag(TagGerado))
            {
                It->Destroy();
            }
        }
    }
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
    if (!Mundo || !MalhaCubo) return;

    FRandomStream Aleatorio(Semente);

    // ------------------------------------------------------------------
    // Grade de arestas
    //  ArestasX: paredes "verticais" (separam X-1 de X), (Colunas+1) x Linhas
    //  ArestasY: paredes "horizontais" (separam Y-1 de Y), Colunas x (Linhas+1)
    // ------------------------------------------------------------------
    TArray<FAresta> ArestasX;
    TArray<FAresta> ArestasY;
    ArestasX.SetNum((Colunas + 1) * Linhas);
    ArestasY.SetNum(Colunas * (Linhas + 1));

    auto AX = [&](int32 X, int32 Y) -> FAresta& { return ArestasX[Y * (Colunas + 1) + X]; };
    auto AY = [&](int32 X, int32 Y) -> FAresta& { return ArestasY[Y * Colunas + X]; };
    auto IndiceCelula = [this](int32 X, int32 Y) { return Y * Colunas + X; };

    // Bordas do mapa sempre fechadas
    for (int32 Y = 0; Y < Linhas; ++Y)
    {
        AX(0, Y).Tipo = ETipoAresta::Solida;
        AX(Colunas, Y).Tipo = ETipoAresta::Solida;
    }
    for (int32 X = 0; X < Colunas; ++X)
    {
        AY(X, 0).Tipo = ETipoAresta::Solida;
        AY(X, Linhas).Tipo = ETipoAresta::Solida;
    }

    // Sorteia um vao (porta, passagem ou abertura total) numa posicao aleatoria da parede
    auto SortearVao = [&](FAresta& Aresta)
    {
        const float R = Aleatorio.FRand();
        float Largura = LarguraPorta;

        if (R < ChanceAbertaTotal)
        {
            Aresta.Tipo = ETipoAresta::Aberta;
            Aresta.Deslocamento = 0.f;
            return;
        }
        if (R < ChanceAbertaTotal + ChancePassagem)
        {
            Aresta.Tipo = ETipoAresta::Passagem;
            Largura = LarguraPassagem;
        }
        else
        {
            Aresta.Tipo = ETipoAresta::Porta;
        }

        // Portas fora do centro deixam o lugar menos "quadriculado"
        const float Folga = FMath::Max(0.f, Meio - Largura * 0.5f - SobraMinima);
        Aresta.Deslocamento = Aleatorio.FRandRange(-Folga, Folga);
    };

    // ------------------------------------------------------------------
    // 1) Divisao recursiva: paredes longas com vaos, criando salas
    // ------------------------------------------------------------------
    const int32 Minimo = FMath::Max(1, TamanhoMinimoSala);

    TArray<FRegiao> Pilha;
    Pilha.Push({ 0, 0, Colunas, Linhas });

    while (Pilha.Num() > 0)
    {
        const FRegiao R = Pilha.Pop();

        const bool bPodeVertical = R.W >= Minimo * 2;
        const bool bPodeHorizontal = R.H >= Minimo * 2;
        if (!bPodeVertical && !bPodeHorizontal) continue;

        // Areas pequenas o suficiente podem virar uma sala inteira
        if (R.W <= TamanhoMaximoSala && R.H <= TamanhoMaximoSala && Aleatorio.FRand() < ChanceSalaGrande)
        {
            continue;
        }

        bool bVertical = bPodeVertical;
        if (bPodeVertical && bPodeHorizontal)
        {
            bVertical = (R.W == R.H) ? (Aleatorio.FRand() < 0.5f) : (R.W > R.H);
        }

        if (bVertical)
        {
            const int32 K = Aleatorio.RandRange(Minimo, R.W - Minimo);
            const int32 LinhaX = R.X + K;
            const int32 VaoPrincipal = Aleatorio.RandRange(R.Y, R.Y + R.H - 1);

            for (int32 Y = R.Y; Y < R.Y + R.H; ++Y)
            {
                FAresta& A = AX(LinhaX, Y);
                A.Tipo = ETipoAresta::Solida;
                A.Deslocamento = 0.f;
                if (Y == VaoPrincipal || Aleatorio.FRand() < ChancePortaExtra)
                {
                    SortearVao(A);
                }
            }

            Pilha.Push({ R.X, R.Y, K, R.H });
            Pilha.Push({ LinhaX, R.Y, R.W - K, R.H });
        }
        else
        {
            const int32 K = Aleatorio.RandRange(Minimo, R.H - Minimo);
            const int32 LinhaY = R.Y + K;
            const int32 VaoPrincipal = Aleatorio.RandRange(R.X, R.X + R.W - 1);

            for (int32 X = R.X; X < R.X + R.W; ++X)
            {
                FAresta& A = AY(X, LinhaY);
                A.Tipo = ETipoAresta::Solida;
                A.Deslocamento = 0.f;
                if (X == VaoPrincipal || Aleatorio.FRand() < ChancePortaExtra)
                {
                    SortearVao(A);
                }
            }

            Pilha.Push({ R.X, R.Y, R.W, K });
            Pilha.Push({ R.X, LinhaY, R.W, R.H - K });
        }
    }

    // ------------------------------------------------------------------
    // 2) Saloes: areas grandes e abertas (so aqui podem existir pilares)
    // ------------------------------------------------------------------
    TArray<bool> VerticeEmSalao;   // cantos internos dos saloes, (Colunas+1) x (Linhas+1)
    VerticeEmSalao.Init(false, (Colunas + 1) * (Linhas + 1));
    auto IndiceVertice = [this](int32 X, int32 Y) { return Y * (Colunas + 1) + X; };

    for (int32 S = 0; S < NumeroDeSaloes; ++S)
    {
        const int32 Largura = FMath::Min(Aleatorio.RandRange(TamanhoSalaoMin, FMath::Max(TamanhoSalaoMin, TamanhoSalaoMax)), Colunas);
        const int32 Altura = FMath::Min(Aleatorio.RandRange(TamanhoSalaoMin, FMath::Max(TamanhoSalaoMin, TamanhoSalaoMax)), Linhas);
        const int32 X0 = Aleatorio.RandRange(0, Colunas - Largura);
        const int32 Y0 = Aleatorio.RandRange(0, Linhas - Altura);

        for (int32 Y = Y0; Y < Y0 + Altura; ++Y)
        {
            for (int32 X = X0 + 1; X < X0 + Largura; ++X)
            {
                AX(X, Y) = FAresta();
            }
        }
        for (int32 Y = Y0 + 1; Y < Y0 + Altura; ++Y)
        {
            for (int32 X = X0; X < X0 + Largura; ++X)
            {
                AY(X, Y) = FAresta();
            }
        }
        for (int32 Y = Y0 + 1; Y < Y0 + Altura; ++Y)
        {
            for (int32 X = X0 + 1; X < X0 + Largura; ++X)
            {
                VerticeEmSalao[IndiceVertice(X, Y)] = true;
            }
        }
    }

    // ------------------------------------------------------------------
    // 2b) Limpeza: remove pedacos de parede soltos (sem encostar em nada nas duas pontas).
    //     Sao eles que deixam o mapa com cara de labirinto.
    // ------------------------------------------------------------------
    auto Existe = [&](bool bLinhaX, int32 X, int32 Y) -> bool
    {
        if (bLinhaX)
        {
            if (X < 0 || X > Colunas || Y < 0 || Y >= Linhas) return false;
            return AX(X, Y).Tipo != ETipoAresta::Aberta;
        }
        if (X < 0 || X >= Colunas || Y < 0 || Y > Linhas) return false;
        return AY(X, Y).Tipo != ETipoAresta::Aberta;
    };

    // Quantas paredes (alem da propria) encostam no canto (VX, VY)
    auto ConexoesNoCanto = [&](int32 VX, int32 VY, bool bIgnorarX, int32 IX, int32 IY) -> int32
    {
        int32 Total = 0;
        if (!(bIgnorarX && IX == VX && IY == VY - 1) && Existe(true, VX, VY - 1)) ++Total;
        if (!(bIgnorarX && IX == VX && IY == VY) && Existe(true, VX, VY)) ++Total;
        if (!(!bIgnorarX && IX == VX - 1 && IY == VY) && Existe(false, VX - 1, VY)) ++Total;
        if (!(!bIgnorarX && IX == VX && IY == VY) && Existe(false, VX, VY)) ++Total;
        return Total;
    };

    for (int32 Passada = 0; Passada < 2; ++Passada)
    {
        for (int32 Y = 0; Y < Linhas; ++Y)
        {
            for (int32 X = 1; X < Colunas; ++X)
            {
                if (AX(X, Y).Tipo == ETipoAresta::Aberta) continue;
                if (ConexoesNoCanto(X, Y, true, X, Y) == 0 && ConexoesNoCanto(X, Y + 1, true, X, Y) == 0)
                {
                    AX(X, Y) = FAresta();
                }
            }
        }
        for (int32 Y = 1; Y < Linhas; ++Y)
        {
            for (int32 X = 0; X < Colunas; ++X)
            {
                if (AY(X, Y).Tipo == ETipoAresta::Aberta) continue;
                if (ConexoesNoCanto(X, Y, false, X, Y) == 0 && ConexoesNoCanto(X + 1, Y, false, X, Y) == 0)
                {
                    AY(X, Y) = FAresta();
                }
            }
        }
    }

    // Poucas paredes soltas propositais, so dentro dos saloes (raras = estranhas)
    for (int32 Y = 1; Y < Linhas; ++Y)
    {
        for (int32 X = 1; X < Colunas; ++X)
        {
            if (VerticeEmSalao[IndiceVertice(X, Y)] && Aleatorio.FRand() < ChanceParedeSolta)
            {
                FAresta& A = (Aleatorio.FRand() < 0.5f) ? AX(X, Y) : AY(X, Y);
                A.Tipo = ETipoAresta::Solida;
                A.Deslocamento = 0.f;
            }
        }
    }

    // ------------------------------------------------------------------
    // 3) Corredores longos de portas alinhadas + fator de luz
    // ------------------------------------------------------------------
    TArray<float> FatorLuz;
    FatorLuz.Init(1.f, Colunas * Linhas);

    for (int32 C = 0; C < NumeroCorredoresLongos; ++C)
    {
        const bool bAoLongoDeX = Aleatorio.FRand() < 0.5f;
        const int32 Limite = bAoLongoDeX ? Colunas : Linhas;
        const int32 Comprimento = FMath::Min(
            Aleatorio.RandRange(ComprimentoCorredorMin, FMath::Max(ComprimentoCorredorMin, ComprimentoCorredorMax)),
            Limite);
        if (Comprimento < 2) continue;

        const int32 X0 = bAoLongoDeX ? Aleatorio.RandRange(0, Colunas - Comprimento) : Aleatorio.RandRange(0, Colunas - 1);
        const int32 Y0 = bAoLongoDeX ? Aleatorio.RandRange(0, Linhas - 1) : Aleatorio.RandRange(0, Linhas - Comprimento);
        const bool bEscureceNoInicio = Aleatorio.FRand() < 0.5f;

        for (int32 I = 0; I < Comprimento; ++I)
        {
            const int32 X = bAoLongoDeX ? X0 + I : X0;
            const int32 Y = bAoLongoDeX ? Y0 : Y0 + I;

            // Parede entre esta celula e a proxima: vira porta centralizada (alinhada com as outras)
            if (I < Comprimento - 1)
            {
                FAresta& A = bAoLongoDeX ? AX(X + 1, Y) : AY(X, Y + 1);
                if (A.Tipo != ETipoAresta::Aberta)
                {
                    A.Tipo = ETipoAresta::Porta;
                    A.Deslocamento = 0.f;
                }
            }

            float Progresso = static_cast<float>(I) / (Comprimento - 1);
            if (bEscureceNoInicio) Progresso = 1.f - Progresso;

            float& Fator = FatorLuz[IndiceCelula(X, Y)];
            Fator = FMath::Min(Fator, 1.f - Progresso * EscurecimentoCorredor);
        }
    }

    // ------------------------------------------------------------------
    // 4) Zonas escuras
    // ------------------------------------------------------------------
    for (int32 Z = 0; Z < NumeroZonasEscuras; ++Z)
    {
        const int32 CX = Aleatorio.RandRange(0, Colunas - 1);
        const int32 CY = Aleatorio.RandRange(0, Linhas - 1);
        const int32 Raio = FMath::CeilToInt(RaioZonaEscura);

        for (int32 Y = FMath::Max(0, CY - Raio); Y <= FMath::Min(Linhas - 1, CY + Raio); ++Y)
        {
            for (int32 X = FMath::Max(0, CX - Raio); X <= FMath::Min(Colunas - 1, CX + Raio); ++X)
            {
                const float Distancia = FVector2D::Distance(FVector2D(X, Y), FVector2D(CX, CY));
                if (Distancia >= RaioZonaEscura) continue;

                const float T = Distancia / RaioZonaEscura;   // 0 no centro, 1 na borda
                float& Fator = FatorLuz[IndiceCelula(X, Y)];
                Fator = FMath::Min(Fator, T * T);
            }
        }
    }

    FatorLuz[IndiceCelula(0, 0)] = 1.f;   // o jogador sempre comeca na luz

    // ------------------------------------------------------------------
    // 5) Portas emparedadas: so em paredes solidas internas (nao quebra caminhos)
    // ------------------------------------------------------------------
    auto TalvezPortaFalsa = [&](FAresta& A, bool bNoMeioDeParede)
    {
        if (A.Tipo == ETipoAresta::Solida && Aleatorio.FRand() < ChancePortaFalsa && bNoMeioDeParede)
        {
            A.Tipo = ETipoAresta::PortaFalsa;
            const float Folga = FMath::Max(0.f, Meio - LarguraPorta * 0.5f - SobraMinima);
            A.Deslocamento = Aleatorio.FRandRange(-Folga, Folga);
        }
    };
    // So no meio de paredes longas (com parede continuando dos dois lados)
    for (int32 Y = 1; Y < Linhas - 1; ++Y)
        for (int32 X = 1; X < Colunas; ++X)
            TalvezPortaFalsa(AX(X, Y), AX(X, Y - 1).Tipo == ETipoAresta::Solida && AX(X, Y + 1).Tipo == ETipoAresta::Solida);
    for (int32 Y = 1; Y < Linhas; ++Y)
        for (int32 X = 1; X < Colunas - 1; ++X)
            TalvezPortaFalsa(AY(X, Y), AY(X - 1, Y).Tipo == ETipoAresta::Solida && AY(X + 1, Y).Tipo == ETipoAresta::Solida);

    // ------------------------------------------------------------------
    // 6) Geometria
    // ------------------------------------------------------------------
    Paredes->SetMaterial(0, MaterialParede);
    Rodapes->SetMaterial(0, MaterialRodape ? MaterialRodape.Get() : MaterialParede.Get());
    Pilares->SetMaterial(0, MaterialPilar ? MaterialPilar.Get() : MaterialParede.Get());

    auto Caixa = [](UInstancedStaticMeshComponent* Componente, const FVector& Centro, const FVector& TamanhoCm)
    {
        Componente->AddInstance(FTransform(FRotator::ZeroRotator, Centro, TamanhoCm / 100.f), false);
    };

    // Um pedaco de parede ao longo da aresta, de S0 a S1 (cm, relativo ao centro da aresta)
    auto Segmento = [&](bool bLinhaX, const FVector2D& Centro, float S0, float S1, float Z0, float Z1, float Grossura, bool bComRodape)
    {
        const float Comprimento = S1 - S0;
        if (Comprimento < 1.f || Z1 - Z0 < 1.f) return;

        const float S = (S0 + S1) * 0.5f;
        const float ZC = (Z0 + Z1) * 0.5f;

        if (bLinhaX)
        {
            Caixa(Paredes, FVector(Centro.X, Centro.Y + S, ZC), FVector(Grossura, Comprimento, Z1 - Z0));
        }
        else
        {
            Caixa(Paredes, FVector(Centro.X + S, Centro.Y, ZC), FVector(Comprimento, Grossura, Z1 - Z0));
        }

        if (!bComRodape) return;

        const float Lados[2] = { -1.f, 1.f };
        for (const float Lado : Lados)
        {
            const float Afastamento = Lado * (Grossura * 0.5f + EspessuraRodape * 0.5f);
            if (bLinhaX)
            {
                Caixa(Rodapes, FVector(Centro.X + Afastamento, Centro.Y + S, AlturaRodape * 0.5f),
                    FVector(EspessuraRodape, Comprimento, AlturaRodape));
            }
            else
            {
                Caixa(Rodapes, FVector(Centro.X + S, Centro.Y + Afastamento, AlturaRodape * 0.5f),
                    FVector(Comprimento, EspessuraRodape, AlturaRodape));
            }
        }
    };

    auto EmitirAresta = [&](bool bLinhaX, const FVector2D& Centro, const FAresta& A)
    {
        const float Inicio = -Meio - Espessura * 0.5f;   // um pouco a mais para fechar os cantos
        const float Fim = Meio + Espessura * 0.5f;

        switch (A.Tipo)
        {
        case ETipoAresta::Aberta:
            return;

        case ETipoAresta::Solida:
            Segmento(bLinhaX, Centro, Inicio, Fim, 0.f, AlturaTeto, Espessura, true);
            return;

        default:
        {
            const bool bPassagem = (A.Tipo == ETipoAresta::Passagem);
            const float Largura = bPassagem ? LarguraPassagem : LarguraPorta;
            const float AlturaVao = bPassagem ? AlturaPassagem : AlturaPorta;
            const float V0 = A.Deslocamento - Largura * 0.5f;
            const float V1 = A.Deslocamento + Largura * 0.5f;

            Segmento(bLinhaX, Centro, Inicio, V0, 0.f, AlturaTeto, Espessura, true);    // lado esquerdo
            Segmento(bLinhaX, Centro, V1, Fim, 0.f, AlturaTeto, Espessura, true);       // lado direito
            Segmento(bLinhaX, Centro, V0, V1, AlturaVao, AlturaTeto, Espessura, false); // parede em cima

            if (A.Tipo == ETipoAresta::PortaFalsa)
            {
                // Fechamento recuado: os dois lados veem um vao de porta que nao leva a lugar nenhum
                Segmento(bLinhaX, Centro, V0, V1, 0.f, AlturaVao, Espessura * 0.4f, false);
            }
            return;
        }
        }
    };

    for (int32 Y = 0; Y < Linhas; ++Y)
    {
        for (int32 X = 0; X <= Colunas; ++X)
        {
            EmitirAresta(true, FVector2D((X - 0.5f) * TamanhoCelula, Y * TamanhoCelula), AX(X, Y));
        }
    }
    for (int32 Y = 0; Y <= Linhas; ++Y)
    {
        for (int32 X = 0; X < Colunas; ++X)
        {
            EmitirAresta(false, FVector2D(X * TamanhoCelula, (Y - 0.5f) * TamanhoCelula), AY(X, Y));
        }
    }

    // Pilares nos cantos totalmente abertos
    for (int32 Y = 1; Y < Linhas; ++Y)
    {
        for (int32 X = 1; X < Colunas; ++X)
        {
            if (!VerticeEmSalao[IndiceVertice(X, Y)]) continue;

            const bool bCantoAberto =
                AX(X, Y - 1).Tipo == ETipoAresta::Aberta && AX(X, Y).Tipo == ETipoAresta::Aberta &&
                AY(X - 1, Y).Tipo == ETipoAresta::Aberta && AY(X, Y).Tipo == ETipoAresta::Aberta;

            if (bCantoAberto && Aleatorio.FRand() < ChancePilar)
            {
                Caixa(Pilares, FVector((X - 0.5f) * TamanhoCelula, (Y - 0.5f) * TamanhoCelula, AlturaTeto * 0.5f),
                    FVector(60.f, 60.f, AlturaTeto));
            }
        }
    }

    // Piso e teto: uma laje unica cobrindo o mapa (texturas alinhadas ao mundo, sem emendas)
    const FVector CentroMapa((Colunas - 1) * Meio, (Linhas - 1) * Meio, 0.f);
    const FVector TamanhoLaje(Colunas * TamanhoCelula + Espessura, Linhas * TamanhoCelula + Espessura, Espessura);

    Piso->Modify();
    Piso->SetRelativeLocation(CentroMapa + FVector(0.f, 0.f, -Espessura * 0.5f));
    Piso->SetRelativeScale3D(TamanhoLaje / 100.f);
    Piso->SetMaterial(0, MaterialPiso);

    Teto->Modify();
    Teto->SetRelativeLocation(CentroMapa + FVector(0.f, 0.f, AlturaTeto + Espessura * 0.5f));
    Teto->SetRelativeScale3D(TamanhoLaje / 100.f);
    Teto->SetMaterial(0, MaterialTeto);

    // ------------------------------------------------------------------
    // 7) Lampadas: acesas, morrendo (piscam muito) ou queimadas
    // ------------------------------------------------------------------
    if (ClasseLampada)
    {
        for (int32 Y = 0; Y < Linhas; ++Y)
        {
            for (int32 X = 0; X < Colunas; ++X)
            {
                const float Fator = FatorLuz[IndiceCelula(X, Y)];
                const bool bInicio = (X == 0 && Y == 0);

                const bool bAcesa = bInicio || Aleatorio.FRand() < ChanceLampada * Fator;
                const bool bQueimada = !bAcesa && Fator < 0.6f && Aleatorio.FRand() < ChanceLampadaQueimada;
                if (!bAcesa && !bQueimada) continue;

                const FVector Local = GetActorTransform().TransformPosition(
                    FVector(X * TamanhoCelula, Y * TamanhoCelula, AlturaTeto - 1.f));
                const FTransform Transform(FRotator::ZeroRotator, Local);

                ALampada* Lampada = Mundo->SpawnActorDeferred<ALampada>(ClasseLampada, Transform, this);
                if (!Lampada) continue;

                const bool bMorrendo = bAcesa && !bInicio && Fator < LimiteLampadaMorrendo
                    && Aleatorio.FRand() < ChanceLampadaMorrendo;
                if (bQueimada)
                {
                    Lampada->ConfigurarEstado(true, 0.f);
                }
                else if (bMorrendo)
                {
                    Lampada->ConfigurarEstado(false, ChanceRajadaMorrendo);
                }
                else
                {
                    Lampada->ConfigurarEstado(false, 0.f);   // estavel: nunca pisca
                }

                Lampada->Tags.Add(TagGerado);
                Lampada->FinishSpawning(Transform);
#if WITH_EDITOR
                Lampada->SetFolderPath(TEXT("Labirinto/Lampadas"));
#endif
                Lampadas.Add(Lampada);
            }
        }
    }

    // ------------------------------------------------------------------
    // 8) PlayerStart na primeira celula, olhando para um lado aberto
    // ------------------------------------------------------------------
    float Yaw = 0.f;
    if (AX(1, 0).Tipo == ETipoAresta::Solida && AY(0, 1).Tipo != ETipoAresta::Solida)
    {
        Yaw = 90.f;
    }

    for (TActorIterator<APlayerStart> It(Mundo); It; ++It)
    {
        It->SetActorLocationAndRotation(GetActorTransform().TransformPosition(FVector(0.f, 0.f, 100.f)), FRotator(0.f, Yaw, 0.f));
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("GeradorLabirinto: %d paredes, %d lampadas (semente %d)."),
        Paredes->GetInstanceCount(), Lampadas.Num(), Semente);
}
