#include <WiFi.h>
#include <Arduino.h>

String ConexaoWifi(const char *ssid, const char *password, uint8_t ledWifi)
{
    Serial.println();
    Serial.print("Conectando ao WiFi: ");
    Serial.println(ssid);

    pinMode(ledWifi, OUTPUT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    bool estadoLed = false;
    unsigned long tempoAnterior = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - tempoAnterior >= 500)
        {
            tempoAnterior = millis();

            estadoLed = !estadoLed;
            digitalWrite(ledWifi, estadoLed);
        }

        delay(1); // Apenas para não ocupar 100% da CPU
    }

    // Conectou
    digitalWrite(ledWifi, HIGH);

    Serial.println("\nWiFi conectado!");
    Serial.print("IP do ESP: ");
    Serial.println(WiFi.localIP());

    return WiFi.localIP().toString();
}