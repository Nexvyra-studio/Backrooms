#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interagivel.generated.h"

class APawn;

// Qualquer coisa que o jogador pode usar com a tecla de interagir (E)
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteragivel : public UInterface
{
    GENERATED_BODY()
};

class BACKROOMS_API IInteragivel
{
    GENERATED_BODY()

public:
    // Sempre chamado no SERVIDOR
    virtual void Interagir(APawn* Quem) = 0;

    virtual bool PodeInteragir() const { return true; }

    // Chamado SO na maquina do jogador quando ele mira/deixa de mirar (mostrar a dica "E")
    virtual void DefinirDestaque(bool bDestacado) {}
};
