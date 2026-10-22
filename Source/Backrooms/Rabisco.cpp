#include "Rabisco.h"
#include "Components/TextRenderComponent.h"

ARabisco::ARabisco()
{
    PrimaryActorTick.bCanEverTick = false;

    Escrita = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Escrita"));
    RootComponent = Escrita;
    Escrita->SetHorizontalAlignment(EHTA_Center);
    Escrita->SetVerticalAlignment(EVRTA_TextCenter);
}

void ARabisco::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Configurar(Texto, Tamanho, Cor);
}

void ARabisco::Configurar(const FText& NovoTexto, float NovoTamanho, FColor NovaCor)
{
    Texto = NovoTexto;
    Tamanho = NovoTamanho;
    Cor = NovaCor;

    Escrita->SetText(Texto);
    Escrita->SetWorldSize(Tamanho);
    Escrita->SetTextRenderColor(Cor);
}
