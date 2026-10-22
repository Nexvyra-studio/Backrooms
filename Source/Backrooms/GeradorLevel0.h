#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeradorLevel0.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ALampada;
class AQuadroLuz;
class AParedeSaida;
class ABilhete;
class AEnigmaLevel0;
class AAmbienteLevel0;
class ADiretorSustos;

// Gerador do LEVEL 0: labirinto de corredores longos e iguais, salas de vez em quando,
// luz fluorescente forte e uniforme. A mesma semente gera o mesmo mapa (importante no multiplayer).
//
// Como funciona:
//  1) Labirinto "escavado" com preferencia por seguir reto (corredores longos).
//  2) Algumas paredes extras sao abertas (caminhos em circulo: o jogador se perde).
//  3) Salas abertas espalhadas, as vezes com colunas.
//  4) Vaos de porta em paredes aleatorias.
//  5) Muitas lampadas de painel; poucas queimadas; uma zona mais escura e estranha.
//  6) Cria o Diretor de Sustos no mapa, se ainda nao existir.
UCLASS()
class BACKROOMS_API AGeradorLevel0 : public AActor
{
    GENERATED_BODY()

public:
    AGeradorLevel0();

    // ================= Mapa =================
    UPROPERTY(EditAnywhere, Category = "Level0", meta = (ClampMin = "4", ClampMax = "100"))
    int32 Colunas = 26;

    UPROPERTY(EditAnywhere, Category = "Level0", meta = (ClampMin = "4", ClampMax = "100"))
    int32 Linhas = 26;

    UPROPERTY(EditAnywhere, Category = "Level0")
    int32 Semente = 2026;

    // Largura de cada corredor/celula (cm). 360 = 6 placas de forro de 60 cm
    // (com o MI_Teto em TamanhoTextura 300, paredes e lampadas caem certinho nas linhas do forro)
    UPROPERTY(EditAnywhere, Category = "Level0", meta = (ClampMin = "200", ClampMax = "600"))
    float TamanhoCelula = 360.f;

    // Pe-direito baixo deixa o ambiente opressivo
    UPROPERTY(EditAnywhere, Category = "Level0", meta = (ClampMin = "220", ClampMax = "500"))
    float AlturaTeto = 270.f;

    UPROPERTY(EditAnywhere, Category = "Level0", meta = (ClampMin = "5", ClampMax = "50"))
    float EspessuraParede = 16.f;

    // ================= Labirinto =================
    // Chance de o corredor continuar reto (mais alto = corredores mais longos)
    UPROPERTY(EditAnywhere, Category = "Level0|Labirinto", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceSeguirReto = 0.72f;

    // Paredes extras abertas: cria caminhos em circulo (voce volta ao mesmo lugar sem perceber)
    UPROPERTY(EditAnywhere, Category = "Level0|Labirinto", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceAbrirExtra = 0.1f;

    // Paredes que ganham um vao de porta (sem porta)
    UPROPERTY(EditAnywhere, Category = "Level0|Labirinto", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceVao = 0.03f;

    UPROPERTY(EditAnywhere, Category = "Level0|Labirinto")
    float LarguraVao = 110.f;

    UPROPERTY(EditAnywhere, Category = "Level0|Labirinto")
    float AlturaVao = 215.f;

    // ================= Salas =================
    UPROPERTY(EditAnywhere, Category = "Level0|Salas", meta = (ClampMin = "0"))
    int32 NumeroSalas = 5;

    UPROPERTY(EditAnywhere, Category = "Level0|Salas", meta = (ClampMin = "2"))
    int32 TamanhoSalaMin = 2;

    UPROPERTY(EditAnywhere, Category = "Level0|Salas", meta = (ClampMin = "2"))
    int32 TamanhoSalaMax = 4;

    // Colunas soltas dentro das salas
    UPROPERTY(EditAnywhere, Category = "Level0|Salas", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChancePilar = 0.04f;

    UPROPERTY(EditAnywhere, Category = "Level0|Salas")
    float LadoPilar = 50.f;

    // ================= Luz =================
    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampada = 0.85f;

    // Das lampadas acesas, quantas tem luz de verdade (o resto so brilha). Menos = mais FPS
    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float FracaoLuzReal = 0.4f;

    // Fora da zona escura: luminarias apagadas de vez em quando
    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampadaQueimada = 0.05f;

    // Fora da zona escura: lampadas que piscam sozinhas
    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceLampadaPiscando = 0.03f;

    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceRajadaPiscando = 0.03f;

    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "0"))
    int32 NumeroZonasEscuras = 1;

    // Em celulas
    UPROPERTY(EditAnywhere, Category = "Level0|Luz", meta = (ClampMin = "1.0"))
    float RaioZonaEscura = 3.f;

    // ================= Visual =================
    // ================= Classes que o gerador coloca no mapa =================
    // Troque pelos Blueprints (BP_...) para ajustar tudo pelo editor, sem mexer no C++
    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<ALampada> ClasseLampada;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<AQuadroLuz> ClasseQuadro;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<AParedeSaida> ClasseSaida;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<ABilhete> ClasseBilhete;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<AEnigmaLevel0> ClasseEnigma;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<AAmbienteLevel0> ClasseAmbiente;

    UPROPERTY(EditAnywhere, Category = "Level0|Classes")
    TSubclassOf<ADiretorSustos> ClasseDiretor;

    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    TObjectPtr<UMaterialInterface> MaterialParede;

    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    TObjectPtr<UMaterialInterface> MaterialPiso;

    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    TObjectPtr<UMaterialInterface> MaterialTeto;

    // Vazio = usa o material da parede
    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    TObjectPtr<UMaterialInterface> MaterialRodape;

    // Desloca a lampada meia placa para o lado, para encaixar nas linhas do forro
    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    float DeslocamentoLampada = 30.f;

    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    float AlturaRodape = 10.f;

    UPROPERTY(EditAnywhere, Category = "Level0|Visual")
    float EspessuraRodape = 1.5f;

    // ================= Detalhes (realismo) =================
    // Tomadas velhas nas paredes (chance por parede)
    UPROPERTY(EditAnywhere, Category = "Level0|Detalhes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceTomada = 0.07f;

    // Grelhas de ventilacao no forro (chance por celula)
    UPROPERTY(EditAnywhere, Category = "Level0|Detalhes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ChanceGrelha = 0.06f;

    UPROPERTY(EditAnywhere, Category = "Level0|Detalhes")
    TObjectPtr<UMaterialInterface> MaterialTomada;

    UPROPERTY(EditAnywhere, Category = "Level0|Detalhes")
    TObjectPtr<UMaterialInterface> MaterialGrelha;

    // ================= Sustos =================
    // Cria um Diretor de Sustos no mapa ao gerar (se nao houver um)
    UPROPERTY(EditAnywhere, Category = "Level0|Sustos")
    bool bCriarDiretorSustos = true;

    // ================= Botoes =================
    UFUNCTION(CallInEditor, Category = "Level0")
    void Gerar();

    UFUNCTION(CallInEditor, Category = "Level0")
    void NovaSemente();

    UFUNCTION(CallInEditor, Category = "Level0")
    void Limpar();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Paredes;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Rodapes;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Pilares;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Tomadas;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UInstancedStaticMeshComponent> Grelhas;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UStaticMeshComponent> Piso;

    UPROPERTY(VisibleAnywhere, Category = "Level0|Componentes")
    TObjectPtr<UStaticMeshComponent> Teto;

private:
    UPROPERTY()
    TObjectPtr<UStaticMesh> MalhaCubo;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> Lampadas;
};
