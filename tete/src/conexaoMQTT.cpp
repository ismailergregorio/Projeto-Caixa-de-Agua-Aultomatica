#include <PubSubClient.h>
#include <Arduino.h>

bool ConexaoMQTT(PubSubClient &client,
                 const char *mqtt_server,
                 int mqtt_port,
                 const char *mqtt_user,
                 const char *mqtt_pass,
                 const char *clientId,
                 uint8_t ledMQTT)
{
    pinMode(ledMQTT, OUTPUT);

    client.setServer(mqtt_server, mqtt_port);

    bool estadoLed = false;
    unsigned long tempoAnterior = 0;

    while (!client.connected())
    {
        if (millis() - tempoAnterior >= 500)
        {
            tempoAnterior = millis();
            estadoLed = !estadoLed;
            digitalWrite(ledMQTT, estadoLed);
        }

        Serial.println("Conectando ao MQTT...");

        if (client.connect(clientId, mqtt_user, mqtt_pass))
        {
            digitalWrite(ledMQTT, HIGH);
            Serial.println("MQTT conectado!");
            return true;
        }

        delay(1000);
    }

    return true;
}