#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "dados_mqtt.h"
#include "updateOTA.h"
#include "conexaoWifi.h"
#include "funcoesExecusao.h"
#include <conexaoMQTT.h>

//
// --- DEFINIÇÃO DE PINOS ---
//

// --- RS485 ---
#define RS485_RX 18   // RO do módulo (pino de recepção)
#define RS485_TX 5    // DI do módulo (pino de transmissão)
#define RS485_CTRL 19 // DE+RE (controle TX/RX do módulo)

// --- LEDs DO SISTEMA ---
#define LED_CAIXA_CHEIA 25
#define LED_CAIXA_METADE 26
#define LED_CAIXA_VAZIA 27

#define LED_CONEXAO_MQTT 14
#define LED_CONEXAO_WIFI 12
#define LED_COBEXAO_ARDUINO 13

#define SAIDA_MOTOR 33

// --- Botão ---
#define BTN_MOTOR 21 // Botão para ligar/desligar o motor

#define quantSensores 3

// ----valor inicial
bool valorSensor1 = false;
bool valorSensor2 = false;
bool valorSensor3 = false;

bool estadoBotao = false;
bool valorStadoMotor = false;
int nivel;

//-- Lista Dos Sensores --
bool statusAnterior[quantSensores] = {
    valorSensor1,
    valorSensor2,
    valorSensor3};

bool statusAtual[quantSensores] = {
    valorSensor1,
    valorSensor2,
    valorSensor3};
//---- contrle do pisca----
bool alertaLed = false;

//---- contrle do buttton----

//--- DECLARAÇÃO DE FUNÇOES ----
bool piscaBool();
bool contoladorDeLed(bool, bool);
void atualizarLeds();
bool lerBotao(int pino);
int obterNivel();
void verificaNivel(String linha);
void ajustaNivel(int sensor, int valor);
void ligarMotor();
void desligarMotor();

// --- CONFIG WIFI ---
const char *ssid = "Ismailer Gregorio Oi Fibra 2.4G";
const char *password = "27270404";

// --- CONFIG MQTT ---
const char *mqtt_server = "ubunto-serve.local";
const int mqtt_port = 1883;
const char *mqtt_user = "admin";
const char *mqtt_pass = "123";

String topcoStatusCaixa = "/statusCaixa";
String topcoStatusMotor = "/statusMotor";

String topcoComandoCaixa = "/comandoCaixa";
String topcoComandoMotor = "/comandoMotor";

// Instância da Serial RS485 (UART1)
HardwareSerial rs485Serial(1);

// --- MQTT
WiFiClient espClient;
PubSubClient client(espClient);

String i = "22";

String id = "000" + i;
String deviceId = "ESP32_TESTE" + i;
String nome = "ESP32 TESTE" + i;
String descricao = "Dispositivo de teste";
String ip = WiFi.localIP().toString();
String verssao = "v.0.0.1";
String placa = "ESP32";

DadosMqtt dadosMqtt(&client, id, deviceId, nome, descricao, placa, verssao);

unsigned long ultimoEnvio = 0;
const long intervalo = 5000;

unsigned long tempoInicioSequencia = 0;
unsigned long tempoPasso = 0;

const int intervaloSequencia = 5000;
const int intervaloPasso = 100;

bool sequenciaAtiva = false;

int passoAtual = 1;

String listaDeComandos[] = {
    "/search",
    "/statusB",
    "/wifi",
    "/mqtt",
    "/health1",
    "/health2",
    "/update"};

void enviarValorCaixa(int dados)
{
  JsonDocument doc;

  doc["comando"] = dados;

  char buffer[256];
  serializeJson(doc, buffer);

  client.publish(("iot/" + deviceId + topcoStatusCaixa).c_str(), buffer);
}

void enviarValorMotor(bool dados)
{
  JsonDocument doc;

  doc["comando"] = dados;

  char buffer[256];
  serializeJson(doc, buffer);

  client.publish(("iot/" + deviceId + topcoStatusMotor).c_str(), buffer);
}

void enviarValorTeste(String dados)
{
  JsonDocument doc;

  doc["comando"] = dados;

  char buffer[256];
  serializeJson(doc, buffer);

  client.publish(("iot/" + deviceId + "/testeCO").c_str(), buffer);
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    bool connected = ConexaoMQTT(client,
                                 mqtt_server,
                                 mqtt_port,
                                 mqtt_user,
                                 mqtt_pass,
                                 deviceId.c_str(),
                                 LED_CONEXAO_MQTT);

    if (connected)
    {
      Serial.println("Conectado!");
      int tamanhoLista = sizeof(listaDeComandos) / sizeof(listaDeComandos[0]);

      for (int i = 0; i < tamanhoLista; i++)
      {
        client.subscribe(("app/" + deviceId + listaDeComandos[i]).c_str());
      }

      client.subscribe("app/devices/search");

      client.subscribe(("app/" + deviceId + topcoStatusCaixa).c_str());
      client.subscribe(("app/" + deviceId + topcoStatusMotor).c_str());

      client.subscribe(("app/" + deviceId + topcoComandoMotor).c_str());
      client.subscribe(("app/" + deviceId + topcoComandoCaixa).c_str());

      dadosMqtt.enviarStatusA();
    }
    else
    {
      Serial.print("Falhou. rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 3s...");
      delay(3000);
    }
  }
}

bool deserializacaoJSON(String mensagem)
{
  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, mensagem);

  if (!erro)
  {
    bool comando = doc["comando"];
    return comando;
    Serial.print("Comando: ");
    Serial.println(comando); // 0
  }
  else
  {
    Serial.println(erro.c_str());
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{

  String message = "";
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.print("Mensagem: ");
  Serial.println(message);

  int tamanhoLista = sizeof(listaDeComandos) / sizeof(listaDeComandos[0]);

  for (int i = 0; i < tamanhoLista; i++)
  {
    int valor = String(topic).indexOf(listaDeComandos[i]);

    if (valor != -1)
    {
      return enviaComando(listaDeComandos[i], message, dadosMqtt);
    }
  }
  if (String(topic) == String("app/devices/search"))
  {
    Serial.println("chegou");
  }
  if (String(topic) == String("app/" + deviceId + topcoStatusCaixa))
  {
    enviarValorCaixa(nivel);
  }
  if (String(topic) == String("app/" + deviceId + topcoStatusMotor))
  {
    enviarValorMotor(valorStadoMotor);
  }

  if (String(topic) == String("app/" + deviceId + topcoComandoCaixa))
  {
    enviarValorCaixa(nivel);
  }
  if (String(topic) == String("app/" + deviceId + topcoComandoMotor))
  {
    bool comando = deserializacaoJSON(String(message));

    if (comando)
    {
      ligarMotor();
    }
    else
    {
      desligarMotor();
    }
  }
}
//
// --- RECEBE DADOS DO ARDUINO VIA RS485 ---
//
void RecebimentoDeDados(uint8_t ledArduino)
{
  static unsigned long ultimoRecebimento = 0;
  static unsigned long tempoPisca = 0;
  static bool estadoLed = false;

  if (rs485Serial.available())
  {
    String linha = rs485Serial.readStringUntil('\n');

    ultimoRecebimento = millis();

    digitalWrite(ledArduino, HIGH);

    verificaNivel(linha);
  }

  // Se ficou mais de 2 segundos sem receber dados
  if (millis() - ultimoRecebimento >= 2000)
  {
    if (millis() - tempoPisca >= 500)
    {
      tempoPisca = millis();

      estadoLed = !estadoLed;
      digitalWrite(ledArduino, estadoLed);
    }
  }
}

void setup()
{
  pinMode(LED_CAIXA_CHEIA, OUTPUT);
  pinMode(LED_CAIXA_METADE, OUTPUT);
  pinMode(LED_CAIXA_VAZIA, OUTPUT);
  pinMode(LED_CONEXAO_MQTT, OUTPUT);
  pinMode(LED_CONEXAO_WIFI, OUTPUT);
  pinMode(LED_COBEXAO_ARDUINO, OUTPUT);

  pinMode(SAIDA_MOTOR, OUTPUT);
  pinMode(BTN_MOTOR, INPUT_PULLUP);

  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW); // RS485 em modo RECEBER

  rs485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(100);

  Serial.begin(115200);
  delay(100);

  digitalWrite(LED_CAIXA_VAZIA, LOW);
  digitalWrite(LED_CAIXA_METADE, LOW);
  digitalWrite(LED_CAIXA_CHEIA, LOW);

  digitalWrite(LED_CONEXAO_WIFI, LOW);
  digitalWrite(LED_CONEXAO_MQTT, LOW);
  digitalWrite(LED_COBEXAO_ARDUINO, LOW);

  ConexaoWifi(ssid, password, LED_CONEXAO_WIFI);

  client.setCallback(callback);
 reconnectMQTT();
  // ===== OTA =====
  ArduinoOTA.setHostname("esp32-caixa");
  ArduinoOTA.setPassword("123456");

  ArduinoOTA
      .onStart([]()
               { Serial.println("Iniciando OTA..."); })
      .onEnd([]()
             { Serial.println("\nOTA Finalizado"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progresso: %u%%\r", (progress * 100) / total); })
      .onError([](ota_error_t error)
               { Serial.printf("Erro OTA[%u]: ", error); });

  ArduinoOTA.begin();
  Serial.println("OTA pronto!");
}

void loop()
{
  ArduinoOTA.handle();
  // Se cair WiFi, reconecta
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi desconectado!");
    ConexaoWifi(ssid, password, LED_CONEXAO_WIFI);
  }

  if (!client.connected())
  {
    Serial.println("MQTT desconectado!");
    reconnectMQTT();
  }

  RecebimentoDeDados(LED_COBEXAO_ARDUINO);
  estadoBotao = lerBotao(BTN_MOTOR);

  if (estadoBotao)
  {
    if (valorStadoMotor)
    {
      desligarMotor();
    }
    else
    {
      ligarMotor();
    }
  }

  if (obterNivel() <= 0 && valorStadoMotor)
  {
    desligarMotor();
  }

  client.loop();
  executarSequenciasLoop(tempoInicioSequencia,
                         intervaloSequencia,
                         tempoPasso, intervaloPasso,
                         sequenciaAtiva,
                         passoAtual,
                         dadosMqtt);
  delay(20);
}

//--- funcçoes de controller-----

bool piscaBool()
{
  static bool estado = false;
  static unsigned long tempoAnterior = 0;

  if (millis() - tempoAnterior >= 100)
  {
    tempoAnterior = millis();
    estado = !estado;
  }

  return estado;
}

bool contoladorDeLed(bool controller, bool pisca = false)
{
  if (pisca && controller)
  {
    return piscaBool();
  }
  return controller;
};

void atualizarLeds()
{
  digitalWrite(LED_CAIXA_VAZIA,
               contoladorDeLed(statusAtual[0], alertaLed));

  digitalWrite(LED_CAIXA_METADE,
               contoladorDeLed(statusAtual[1], alertaLed));

  digitalWrite(LED_CAIXA_CHEIA,
               contoladorDeLed(statusAtual[2], alertaLed));
}

bool lerBotao(int pino)
{
  static bool ultimoEstadoBotao = HIGH;
  static bool estadoLeitura = HIGH;
  static unsigned long ultimoTempo = 0;

  const unsigned long debounce = 50;

  bool leitura = digitalRead(pino);

  // Detectou mudança?
  if (leitura != estadoLeitura)
  {
    ultimoTempo = millis();
    estadoLeitura = leitura;
  }

  // Espera estabilizar
  if ((millis() - ultimoTempo) > debounce)
  {
    if (estadoLeitura != ultimoEstadoBotao)
    {
      ultimoEstadoBotao = estadoLeitura;

      // Retorna true apenas no momento em que o botão é pressionado
      if (ultimoEstadoBotao == LOW)
      {
        return true;
      }
    }
  }

  return false;
}

int obterNivel()
{ //              1                  2                  3
  if (statusAtual[0] && !statusAtual[1] && !statusAtual[2])
    return 100;

  if (statusAtual[0] && !statusAtual[1] && statusAtual[2])
    return 50;

  if (statusAtual[0] && statusAtual[1] && statusAtual[2])
    return 10;

  if (!statusAtual[0] && statusAtual[1] && statusAtual[2])
    return 0;

  return -1;
}

void verificaNivel(String linha)
{
  if (!linha.startsWith("#SEN"))
    return;

  int sensor = linha.charAt(5) - '0'; // 1, 2 ou 3
  bool valor = linha.charAt(8) - '0'; // false ou true

  statusAtual[sensor - 1] = valor;

  switch (sensor)
  {
  case 1:
    digitalWrite(LED_CAIXA_VAZIA, contoladorDeLed(valor, alertaLed));
    break;

  case 2:
    digitalWrite(LED_CAIXA_METADE, contoladorDeLed(!valor, alertaLed));
    break;

  case 3:
    digitalWrite(LED_CAIXA_CHEIA, contoladorDeLed(!valor, alertaLed));
    break;
  }

  ajustaNivel(sensor - 1, valor);
  memcpy(statusAnterior, statusAtual, sizeof(statusAtual));
}

void ajustaNivel(int sensor, int valor)
{
  if (statusAnterior[sensor] != statusAtual[sensor])
  {
    nivel = obterNivel();
    enviarValorCaixa(nivel);
  }
}
void ligarMotor()
{
  if (obterNivel() >= 50)
  {
    valorStadoMotor = true;
    alertaLed = valorStadoMotor;
    digitalWrite(SAIDA_MOTOR, valorStadoMotor);
    enviarValorMotor(valorStadoMotor);
  }
}

void desligarMotor()
{
  valorStadoMotor = false;
  alertaLed = valorStadoMotor;
  digitalWrite(SAIDA_MOTOR, valorStadoMotor);
  enviarValorMotor(valorStadoMotor);
}

// void resetarBtn()
// {
//   estadoRetencao = false;
//   // ultimoEstadoBotao = HIGH;
//   // estadoLeitura = HIGH;
// }
