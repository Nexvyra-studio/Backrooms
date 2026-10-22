#pragma once

#include "CoreMinimal.h"
#include "Lampada.h"
#include "LampadaPainel.generated.h"

// Luminaria do Level 0: painel liso e leitoso embutido no forro.
// O C++ define o formato e a luz; materiais e sons sao escolhidos no BP_LampadaPainel.
UCLASS()
class BACKROOMS_API ALampadaPainel : public ALampada
{
    GENERATED_BODY()

public:
    ALampadaPainel();
};
