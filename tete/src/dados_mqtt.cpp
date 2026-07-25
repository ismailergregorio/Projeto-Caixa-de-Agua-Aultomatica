#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "dados_mqtt.h"

DadosMqtt::DadosMqtt(PubSubClient* mqttCliente, String _id, String _deviceId, String _nome, String _descricao,String _placa, String _versao)
{
    client = mqttCliente;
    id = _id;
    deviceId = _deviceId;
    nome = _nome;
    descricao = _descricao;
    placa = _placa;
    versao = _versao;
}

void DadosMqtt::enviarDispositivo()
{
    JsonDocument doc;

    doc["id"] = id;
    doc["deviceId"] = deviceId;
    doc["nome"] = nome;
    doc["descricao"] = descricao;
    doc["ip"] = WiFi.localIP().toString();
    doc["placa"] = placa;

    char buffer[256];
    serializeJson(doc, buffer);

    client->publish(("iot/" + deviceId + "/busca").c_str(), buffer);
}

void DadosMqtt::enviarStatusA()
{
    JsonDocument doc;

    doc["deviceId"] = deviceId;
    doc["status"] = "online";
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["freeHeap"] = ESP.getFreeHeap();

    char buffer[256];
    serializeJson(doc, buffer);

    client->publish(("iot/" + deviceId + "/statusA").c_str(), buffer);
}

void DadosMqtt::enviarStatusB()
{
    JsonDocument doc;

    doc["update"] = 0;
    doc["online"] = true;
    doc["uptime"] = millis() / 1000;

    JsonObject fw = doc["fw"].to<JsonObject>();
    fw["v"] = placa + " " + deviceId + "-" + versao;
    char buffer[256];

    serializeJson(doc, buffer);
    client->publish(("iot/" + deviceId + "/statusB").c_str(), buffer);
}

void DadosMqtt::enviarWifi()
{
    JsonDocument doc;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["ip"] = WiFi.localIP().toString();
    wifi["rssi"] = WiFi.RSSI();
    wifi["connected"] = WiFi.status() == WL_CONNECTED;

    char buffer[256];

    serializeJson(doc, buffer);
    client->publish(("iot/" + deviceId + "/wifi").c_str(), buffer);
}

void DadosMqtt::enviarMQTT()
{
    JsonDocument doc;

    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    mqtt["connected"] = client->connected();

    char buffer[256];

    serializeJson(doc, buffer);
    client->publish(("iot/" + deviceId + "/mqtt").c_str(), buffer);
}

void DadosMqtt::enviarHealth1()
{
    JsonDocument doc;

    uint32_t totalRAM = 81920; // valor típico ESP8266
    uint32_t freeRAM = ESP.getFreeHeap();
    uint32_t usedRAM = totalRAM - freeRAM;
    float percentRAM = (usedRAM * 100.0) / totalRAM;

    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["heapFree"] = freeRAM;
    memory["heapUsed"] = usedRAM;
    memory["heapTotal"] = totalRAM;
    memory["heapPercent"] = percentRAM;
    memory["heapMaxBlock"] = ESP.getFreeHeap();
    memory["heapFragmentation"] = ESP.getMaxAllocHeap();

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));

    client->publish(("iot/" + deviceId + "/health1").c_str(), buffer);
}

void DadosMqtt::enviarHealth2()
{
    JsonDocument doc;

    // uint32_t flashTotal = ESP.getFlashChipRealSize();
    uint32_t flashTotal = ESP.getFlashChipSize();
    uint32_t flashUsed = ESP.getSketchSize();
    float percentFlash = (flashUsed * 100.0) / flashTotal;

    JsonObject flash = doc["flash"].to<JsonObject>();
    flash["total"] = flashTotal;
    flash["used"] = flashUsed;
    flash["percent"] = percentFlash;

    JsonObject reset = doc["reset"].to<JsonObject>();
    // reset["reason"] = ESP.getResetReason();

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));

    client->publish(("iot/" + deviceId + "/health2").c_str(), buffer);
}