#include "Celula.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float Tamanho = 400.f;  // 4 m de lado
    constexpr float Altura = 300.f;  // 3 m de pé-direito
    constexpr float Espessura = 20.f;   // 20 cm (paredes, piso e teto)

    void AplicarMaterial(UStaticMeshComponent* Peca, UMaterialInterface* Material)
    {
        if (Peca && Material)
        {
            Peca->SetMaterial(0, Material);
        }
    }

    void ConfigurarParede(UStaticMeshComponent* Parede, bool bAtiva)
    {
        if (!Parede) return;
        Parede->SetVisibility(bAtiva);
        Parede->SetCollisionEnabled(bAtiva ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }
}

ACelula::ACelula()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Malha = Cubo.Succeeded() ? Cubo.Object : nullptr;

    const float Meio = Tamanho * 0.5f;

    // Piso: topo exatamente em Z = 0
    Piso = CriarPeca(TEXT("Piso"), FVector(0.f, 0.f, -Espessura * 0.5f),
        FVector(Tamanho, Tamanho, Espessura), Malha);

    // Teto: parte de baixo exatamente em Z = Altura
    Teto = CriarPeca(TEXT("Teto"), FVector(0.f, 0.f, Altura + Espessura * 0.5f),
        FVector(Tamanho, Tamanho, Espessura), Malha);

    // Paredes nas bordas da célula (um pouco mais compridas para fechar os cantos)
    const FVector TamanhoNS(Espessura, Tamanho + Espessura, Altura);
    const FVector TamanhoLO(Tamanho + Espessura, Espessura, Altura);

    ParedeNorte = CriarPeca(TEXT("ParedeNorte"), FVector(Meio, 0.f, Altura * 0.5f), TamanhoNS, Malha);
    ParedeSul = CriarPeca(TEXT("ParedeSul"), FVector(-Meio, 0.f, Altura * 0.5f), TamanhoNS, Malha);
    ParedeLeste = CriarPeca(TEXT("ParedeLeste"), FVector(0.f, Meio, Altura * 0.5f), TamanhoLO, Malha);
    ParedeOeste = CriarPeca(TEXT("ParedeOeste"), FVector(0.f, -Meio, Altura * 0.5f), TamanhoLO, Malha);

    // Lâmpada no centro, colada no teto
    Lampada = CreateDefaultSubobject<UChildActorComponent>(TEXT("Lampada"));
    Lampada->SetupAttachment(RootComponent);
    Lampada->SetRelativeLocation(FVector(0.f, 0.f, Altura - 1.f));
}

UStaticMeshComponent* ACelula::CriarPeca(const TCHAR* Nome, const FVector& Local, const FVector& TamanhoCm, UStaticMesh* Malha)
{
    UStaticMeshComponent* Peca = CreateDefaultSubobject<UStaticMeshComponent>(Nome);
    Peca->SetupAttachment(RootComponent);
    Peca->SetRelativeLocation(Local);
    Peca->SetRelativeScale3D(TamanhoCm / 100.f);   // o cubo padrão tem 100 cm
    if (Malha)
    {
        Peca->SetStaticMesh(Malha);
    }
    return Peca;
}

void ACelula::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Materiais
    AplicarMaterial(Piso, MaterialPiso);
    AplicarMaterial(Teto, MaterialTeto);
    AplicarMaterial(ParedeNorte, MaterialParede);
    AplicarMaterial(ParedeSul, MaterialParede);
    AplicarMaterial(ParedeLeste, MaterialParede);
    AplicarMaterial(ParedeOeste, MaterialParede);

    // Paredes ligadas/desligadas
    ConfigurarParede(ParedeNorte, bParedeNorte);
    ConfigurarParede(ParedeSul, bParedeSul);
    ConfigurarParede(ParedeLeste, bParedeLeste);
    ConfigurarParede(ParedeOeste, bParedeOeste);

    // Lâmpada
    UClass* ClasseDesejada = (bTemLampada && ClasseLampada) ? ClasseLampada.Get() : nullptr;
    if (Lampada->GetChildActorClass() != ClasseDesejada)
    {
        Lampada->SetChildActorClass(ClasseDesejada);
    }
}