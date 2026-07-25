#ifndef FUNCOESEXECUSAO_H
#define FUNCOESEXECUSAO_H
#include <Arduino.h>
#include "dados_mqtt.h"
void executarSequenciasLoop(
    unsigned long &tempoInicioSequencia,
    unsigned long intervaloSequencia,
    unsigned long &tempoPasso,
    const int intervaloPasso,
    bool &sequenciaAtiva,
    int &passoAtual,
    DadosMqtt &dadosMqtt);
void enviaComando(String topicMensagem, String message,DadosMqtt dados);
#endif