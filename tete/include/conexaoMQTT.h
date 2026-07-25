#ifndef CONEXAOMQTT_H
#define CONEXAOMQTT_H
#include <Arduino.h>
#include <PubSubClient.h>
bool ConexaoMQTT(PubSubClient &client,
                 const char *mqtt_server,
                 int mqtt_port,
                 const char *mqtt_user,
                 const char *mqtt_pass,
                 const char *clientId,
                 uint8_t ledMQTT);
#endif