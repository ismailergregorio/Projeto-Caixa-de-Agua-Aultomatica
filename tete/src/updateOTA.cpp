#include <HTTPUpdate.h>
#include <ArduinoJson.h>
void executarOTA(String message)
{
    HTTPClient http;
    // cria documento JSON

    StaticJsonDocument<256> doc;

    // desserializa JSON
    DeserializationError erro = deserializeJson(doc, message);

    if (erro)
    {

        Serial.print("Erro JSON: ");
        Serial.println(erro.c_str());

        return;
    }

    String url = doc["dados"]["url"];

    Serial.println("Iniciando OTA...");
    Serial.println(ESP.getFreeHeap());
    Serial.println("URL: " + url);

    WiFiClient client;

    // 🔥 MUITO IMPORTANTE
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    // ESPhttpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return ret = httpUpdate.update(client, "http://192.168.100.5:8081/" + url);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("Erro OTA (%d): %s\n",
                      httpUpdate.getLastError(),
                      httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("Sem atualizações.");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("Atualização concluída!");
        break;
    }
}