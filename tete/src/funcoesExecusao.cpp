#include <Arduino.h>
#include "dados_mqtt.h"
#include "updateOTA.h"
void executarSequenciasLoop(
    unsigned long &tempoInicioSequencia,
    unsigned long intervaloSequencia,
    unsigned long &tempoPasso,
    const int intervaloPasso,
    bool &sequenciaAtiva,
    int &passoAtual,
    DadosMqtt &dadosMqtt)
{
    if (!sequenciaAtiva &&
        millis() - tempoInicioSequencia >= intervaloSequencia)
    {

        sequenciaAtiva = true;

        passoAtual = 1;

        tempoPasso = millis();

        tempoInicioSequencia = millis();
    }

    // executa passos da sequência
    if (sequenciaAtiva &&
        millis() - tempoPasso >= intervaloPasso)
    {

        tempoPasso = millis();

        switch (passoAtual)
        {
        case 1:
            dadosMqtt.enviarStatusB();
            break;

        case 2:
            dadosMqtt.enviarWifi();
            break;

        case 3:
            dadosMqtt.enviarMQTT();
            break;

        case 4:
            break;

        case 5:
            dadosMqtt.enviarHealth1();
            break;

        case 6:
            dadosMqtt.enviarHealth2();
            break;
        }

        passoAtual++;

        // terminou sequência
        if (passoAtual > 6)
        {
            sequenciaAtiva = false;
        }
    }
}

void enviaComando(String topicMensagem, String message, DadosMqtt dados)
{
    if (topicMensagem == "/search")
    {
        dados.enviarDispositivo();
    }

    if (topicMensagem == "/statusA")
    {
        dados.enviarStatusA();
    }

    if (topicMensagem == "/statusB")
    {
        dados.enviarStatusB();
    }

    if (topicMensagem == "/wifi")
    {
        dados.enviarWifi();
    }

    if (topicMensagem == "/mqtt")
    {
        dados.enviarMQTT();
    }

    if (topicMensagem == "/health1")
    {
        dados.enviarHealth1();
    }

    if (topicMensagem == "/health2")
    {
        dados.enviarHealth2();
    }

    if (topicMensagem == "/update")
    {
        executarOTA(message);
    }
}