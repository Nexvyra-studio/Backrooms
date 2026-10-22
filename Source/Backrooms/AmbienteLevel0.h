#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmbienteLevel0.generated.h"

class UAudioComponent;
class USoundBase;

// Som de fundo do Level 0 e a "chegada" do jogador.
// - Ruido de sala constante (ventilacao, predio vazio) por baixo do zumbido das lampadas.
// - Intro: tela preta, o som de cair "atraves" da realidade, e a imagem aparece devagar.
// Cada maquina toca o seu (nao precisa de rede). O gerador do Level 0 coloca um no mapa.
UCLASS()
class BACKROOMS_API AAmbienteLevel0 : public AActor
{
    GENERATED_BODY()

public:
    AAmbienteLevel0();

    // Escolhido no Blueprint (BP_AmbienteLevel0)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambiente")
    TObjectPtr<USoundBase> SomSala;

    UPROPERTY(EditAnywhere, Category = "Ambiente", meta = (ClampMin = "0.0"))
    float VolumeSala = 0.55f;

    // ---------- Intro ----------
    UPROPERTY(EditAnywhere, Category = "Ambiente|Intro")
    bool bIntro = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambiente|Intro")
    TObjectPtr<USoundBase> SomChegada;

    // Tempo de tela preta antes de comecar a clarear (s)
    UPROPERTY(EditAnywhere, Category = "Ambiente|Intro")
    float TempoPreto = 2.2f;

    // Tempo do clarear (s)
    UPROPERTY(EditAnywhere, Category = "Ambiente|Intro")
    float TempoClarear = 4.5f;

    // Abaixa o som de fundo (usado nos sustos de silencio). 1 = normal, 0 = mudo
    void Abafar(float Alvo, float Tempo);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Ambiente")
    TObjectPtr<UAudioComponent> Sala;

private:
    void ComecarIntro();
};
