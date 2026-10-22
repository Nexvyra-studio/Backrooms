#include "InteracaoComponent.h"
#include "Interagivel.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UInteracaoComponent::UInteracaoComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
    SetIsReplicatedByDefault(true);
}

void UInteracaoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // O controle do personagem so existe depois que o jogador "assume" o corpo
    if (!bEntradaLigada)
    {
        TentarLigarEntrada();
        return;
    }
    AtualizarDestaque();
}

void UInteracaoComponent::AtualizarDestaque()
{
    const APawn* Dono = Cast<APawn>(GetOwner());
    if (!Dono || !Dono->IsLocallyControlled()) return;

    AActor* Novo = ProcurarAlvo();
    AActor* Antigo = AlvoDestacado.Get();
    if (Novo == Antigo) return;

    if (IInteragivel* I = Cast<IInteragivel>(Antigo)) I->DefinirDestaque(false);
    if (IInteragivel* I = Cast<IInteragivel>(Novo))   I->DefinirDestaque(true);
    AlvoDestacado = Novo;
}

void UInteracaoComponent::TentarLigarEntrada()
{
    APawn* Dono = Cast<APawn>(GetOwner());
    if (!Dono || !Dono->IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(Dono->GetController());
    UEnhancedInputComponent* Entrada = Cast<UEnhancedInputComponent>(Dono->InputComponent);
    if (!PC || !Entrada) return;

    ULocalPlayer* Local = PC->GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* Subsistema = Local ? Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!Subsistema) return;

    // Sem assets configurados: cria a acao e a tecla em tempo de jogo (provisorio)
    if (!AcaoInteragir || !Mapeamento)
    {
        UE_LOG(LogTemp, Warning, TEXT("InteracaoComponent: IA_Interagir/IMC_Interacao nao configurados, usando a tecla %s provisoria"), *Tecla.ToString());
        AcaoInteragir = NewObject<UInputAction>(this, TEXT("IA_Interagir_Provisorio"));
        AcaoInteragir->ValueType = EInputActionValueType::Boolean;
        Mapeamento = NewObject<UInputMappingContext>(this, TEXT("IMC_Interacao_Provisorio"));
        Mapeamento->MapKey(AcaoInteragir, Tecla);
    }

    Subsistema->AddMappingContext(Mapeamento, 1);
    Entrada->BindAction(AcaoInteragir, ETriggerEvent::Started, this, &UInteracaoComponent::Interagir);

    bEntradaLigada = true;
}

AActor* UInteracaoComponent::ProcurarAlvo() const
{
    const APawn* Dono = Cast<APawn>(GetOwner());
    const AController* Controle = Dono ? Dono->GetController() : nullptr;
    if (!Controle) return nullptr;

    FVector Olho;
    FRotator Direcao;
    Controle->GetPlayerViewPoint(Olho, Direcao);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(Interacao), false, Dono);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Olho, Olho + Direcao.Vector() * Alcance, ECC_Visibility, Params))
    {
        return nullptr;
    }

    AActor* Alvo = Hit.GetActor();
    const IInteragivel* Interagivel = Cast<IInteragivel>(Alvo);
    return (Interagivel && Interagivel->PodeInteragir()) ? Alvo : nullptr;
}

void UInteracaoComponent::Interagir()
{
    if (AActor* Alvo = ProcurarAlvo())
    {
        Server_Interagir(Alvo);
    }
}

void UInteracaoComponent::Server_Interagir_Implementation(AActor* Alvo)
{
    APawn* Dono = Cast<APawn>(GetOwner());
    IInteragivel* Interagivel = Cast<IInteragivel>(Alvo);
    if (!Dono || !Interagivel || !Interagivel->PodeInteragir()) return;

    // Protecao simples: precisa estar perto de verdade
    if (FVector::Dist(Dono->GetActorLocation(), Alvo->GetActorLocation()) > Alcance + 200.f) return;

    Interagivel->Interagir(Dono);
}
