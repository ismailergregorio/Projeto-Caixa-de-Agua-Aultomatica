#ifndef DADOS_MQTT_H
#define DADOS_MQTT_H

#include <PubSubClient.h>
#include <Arduino.h>

class DadosMqtt
{
private:
    PubSubClient* client;
    String id;
    String deviceId;
    String nome;
    String descricao;
    String placa;
    String versao;

public:
    DadosMqtt(PubSubClient* mqttCliente, 
                String _id, 
                String _deviceId, 
                String _nome, 
                String _descricao, 
                String _placa, 
                String _versao);

    void enviarDispositivo();
    void enviarStatusA();
    void enviarStatusB();
    void enviarWifi();
    void enviarMQTT();
    void enviarHealth1();
    void enviarHealth2();
};

#endif