#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

//
// --- CONFIGURAÇÕES DE REDE WIFI ---
//
const char *ssid = "Ismailer Gregorio Oi Fibra 2.4G";
const char *password = "27270404";

//
// --- CONFIGURAÇÕES MQTT ---
//
const char *mqtt_server = "ubunto-serve.local";
const int mqtt_port = 1883;
const char *mqtt_user = "admin";
const char *mqtt_pass = "123";

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

//
// --- VARIÁVEIS MQTT ---
//

// --- VARIÁVEIS MQTT CAIXA ---
String sensorNivelCaixa1 = "caixa/sensor/nivel/1";
String sensorNivelCaixa2 = "caixa/sensor/nivel/2";
String sensorNivelCaixa3 = "caixa/sensor/nivel/3";

String statusCaixa = "caixa/status";    // 0,1,2,3
String comandoCaixa = "caixa/controle"; // 0,1,2,3

// --- VARIÁVEIS MQTT MOTOR ---
String comandoMotor = "motor/controle";
String statusMotor = "motor/status";

// --- VARIÁVEIS MQTT SITE ou SERVIDOR ---
String initSite = "site/init";

// Estados atuais dos sensores (padrão TRUE = desligado)
bool estadoSensorDeNivel1 = false;
bool estadoSensorDeNivel2 = true;
bool estadoSensorDeNivel3 = true;

// Estados anteriores dos sensores (para detectar mudança)
bool estadoSensorDeNivel1Anterior = true;
bool estadoSensorDeNivel2Anterior = false;
bool estadoSensorDeNivel3Anterior = false;

// Estado atual da saída do motor
bool estadoSaidaDoMotor = false;

String nivelDaCaixa;

// Controle do botão (toggle)
bool estadoBTN = false;
bool ultimoEstadoBotao = HIGH;

// Controle de comunicação RS485
unsigned long ultimoRecebimento = 0;
const unsigned long tempoLimite = 1500; // 1.5 segundos sem comunicação = LED apaga
bool dadosChegando = false;

// Instância da Serial RS485 (UART1)
HardwareSerial rs485Serial(1);

// Controle de piscagem (geral)
unsigned long intervalo = 2000;
unsigned long ultimoTempo = 0;

WiFiClient espClient;
PubSubClient client(espClient);

//
// --- VARIÁVEIS PARA PISCA DE LED ---
//
unsigned long ultimoTempo_F = 0;
bool estadoLed = LOW;

//
// --- FUNÇÃO: MONITORA COMUNICAÇÃO COM ARDUINO ---
//
void monitorarComunicacao()
{
  unsigned long agora = millis();

  // Se passaram mais de 1.5s sem dados → consideramos desconectado
  if (agora - ultimoRecebimento > tempoLimite)
  {
    dadosChegando = false;
  }

  // Atualiza o LED correspondente
  digitalWrite(LED_COBEXAO_ARDUINO, dadosChegando ? HIGH : LOW);
}

//
// --- FUNÇÃO: PISCAR QUALQUER LED A CADA 250ms ---
//
void piscaLed(int led)
{
  unsigned long tF = millis();

  if (tF - ultimoTempo_F >= 250)
  {
    estadoLed = !estadoLed;
    digitalWrite(led, estadoLed);
    ultimoTempo_F = tF;
  }
}

//
// --- FUNÇÃO: PISCAGEM DOS LEDs DE NÍVEL QUANDO MOTOR ESTÁ ATIVO ---
//
void sinalisacaoMotor()
{
  unsigned long tF = millis();

  if (tF - ultimoTempo_F >= 250)
  {
    ultimoTempo_F = tF;
    estadoLed = !estadoLed;

    // Desliga todos antes de acender só os necessários
    digitalWrite(LED_CAIXA_VAZIA, LOW);
    digitalWrite(LED_CAIXA_METADE, LOW);
    digitalWrite(LED_CAIXA_CHEIA, LOW);

    // Caixa cheia (0,0,0)
    if (!estadoSensorDeNivel1 && !estadoSensorDeNivel2 && !estadoSensorDeNivel3)
    {
      digitalWrite(LED_CAIXA_CHEIA, estadoLed);
      digitalWrite(LED_CAIXA_METADE, estadoLed);
      digitalWrite(LED_CAIXA_VAZIA, estadoLed);
    }
    // Metade (0,0,1)
    else if (!estadoSensorDeNivel1 && !estadoSensorDeNivel2)
    {
      digitalWrite(LED_CAIXA_METADE, estadoLed);
      digitalWrite(LED_CAIXA_VAZIA, estadoLed);
    }
    // Vazio (0,1,1)
    else if (!estadoSensorDeNivel1)
    {
      digitalWrite(LED_CAIXA_VAZIA, estadoLed);
    }
  }
}

//
// --- FUNÇÃO: PUBLICA ESTADOS DOS SENSORES SE HOUVER MUDANÇA ---
//
void enviaEstadoDoSensor(String resultado)
{
  String r = resultado.substring(0, 7);
  int v = resultado.substring(8).toInt();

  if (r == "#SEN[2]")
  {
    v = (v == 1) ? 0 : 1;
  }

  if(r == "#SEN[3]"){
    v = (v == 1) ? 0 : 1;
  }

  if (r == "#SEN[1]" && v != estadoSensorDeNivel1Anterior)
  {
    estadoSensorDeNivel1 = v;
    client.publish(sensorNivelCaixa1.c_str(), String(v).c_str());
    // client.publish(sensorNivelCaixa1.c_str(), String(1).c_str());
  }
  else if (r == "#SEN[2]" && v != estadoSensorDeNivel2Anterior)
  {
    estadoSensorDeNivel2 = v;
    // client.publish(sensorNivelCaixa1.c_str(), String(1).c_str());
    client.publish(sensorNivelCaixa2.c_str(), String(v).c_str());
  }
  else if (r == "#SEN[3]" && v != estadoSensorDeNivel3Anterior)
  {
    estadoSensorDeNivel3 = v;
    // client.publish(sensorNivelCaixa1.c_str(), String(0).c_str());
    client.publish(sensorNivelCaixa3.c_str(), String(v).c_str());
  }
}

//
// --- FUNÇÃO: AVALIA O NÍVEL DA CAIXA BASEADO NOS SENSORES ---
//
void nivelCaixa()
{
  if (estadoSensorDeNivel1 != estadoSensorDeNivel1Anterior ||
      estadoSensorDeNivel2 != estadoSensorDeNivel2Anterior ||
      estadoSensorDeNivel3 != estadoSensorDeNivel3Anterior)
  {
    estadoSensorDeNivel1Anterior = estadoSensorDeNivel1;
    estadoSensorDeNivel2Anterior = estadoSensorDeNivel2;
    estadoSensorDeNivel3Anterior = estadoSensorDeNivel3;

    String estado = String(estadoSensorDeNivel1) + "," +
                    String(estadoSensorDeNivel2) + "," +
                    String(estadoSensorDeNivel3);

    if (estado == "0,0,1")
    {
      nivelDaCaixa = "1";
    }
    else if (estado == "0,1,1")
    {
      nivelDaCaixa = "2";
    }
    else if (estado == "1,1,1")
    {
      nivelDaCaixa = "3";
    }
    else
    {
      nivelDaCaixa = "0";
    }

    client.publish(statusCaixa.c_str(), nivelDaCaixa.c_str());
    client.publish(comandoCaixa.c_str(), nivelDaCaixa.c_str());
  }
  // Liga/desliga LEDs conforme o estado dos sensores
  digitalWrite(LED_CAIXA_CHEIA, estadoSensorDeNivel3);
  digitalWrite(LED_CAIXA_METADE, estadoSensorDeNivel2);
  digitalWrite(LED_CAIXA_VAZIA, estadoSensorDeNivel1);
}

//
// --- FUNÇÃO: CONTROLE DO MOTOR (COMANDO DO SITE OU BOTÃO) ---
//
void comtroleDoMoto(String mensagem = "")
{
  String v = mensagem;
  if (mensagem == "1" || mensagem == "0")
  {
    digitalWrite(SAIDA_MOTOR, v.toInt());
    estadoBTN = v.toInt();

    // nivelCaixa();
  }
  ultimoEstadoBotao = estadoBTN;
}

void controleAltomaticoMotor(bool estado)
{
  if (estado == 0 && estadoBTN == true)
  {
    digitalWrite(SAIDA_MOTOR, !estadoBTN);
    estadoBTN = !estadoBTN;
    ultimoEstadoBotao = estadoBTN;
    client.publish(statusMotor.c_str(), String(estadoBTN).c_str());
  }
}

//
// --- CALLBACK MQTT (RECEBE MENSAGENS DO BROKER) ---
//
void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  if (String(topic) == initSite && message == "true")
  {
    if (estadoSensorDeNivel1 != estadoSensorDeNivel1Anterior ||
        estadoSensorDeNivel2 != estadoSensorDeNivel2Anterior ||
        estadoSensorDeNivel3 != estadoSensorDeNivel3Anterior)
    {
      client.publish(statusCaixa.c_str(), nivelDaCaixa.c_str());
    }
    client.publish(sensorNivelCaixa1.c_str(), String(estadoSensorDeNivel1).c_str());
    client.publish(sensorNivelCaixa2.c_str(), String(estadoSensorDeNivel2).c_str());
    client.publish(sensorNivelCaixa3.c_str(), String(estadoSensorDeNivel3).c_str());
    client.publish(statusCaixa.c_str(), String(nivelDaCaixa).c_str());
    client.publish(statusMotor.c_str(), String(estadoBTN).c_str());
  }

  if (String(topic) == comandoMotor && message != String(estadoBTN).c_str())
  {
    comtroleDoMoto(message);
    // client.publish(comandoMotor.c_str(), String(estadoBTN).c_str());
    client.publish(statusMotor.c_str(), String(estadoBTN).c_str());
  }
}

//
// --- RECEBE DADOS DO ARDUINO VIA RS485 ---
//
String RecebimentoDeDados()
{
  String dados = "";

  if (rs485Serial.available())
  {
    dados = rs485Serial.readStringUntil('\n');

    // Marca que chegou dado
    dadosChegando = true;

    // Atualiza o tempo da última comunicação
    ultimoRecebimento = millis();
  }

  return dados;
}

//
// --- TENTA RECONEXÃO MQTT ---
//
int tentativa = 0;

void reconnect()
{
  while (!client.connected())
  {
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass))
    {
      digitalWrite(LED_CONEXAO_MQTT, HIGH);
      Serial.println("MQTT conectado");
      client.subscribe(initSite.c_str());
      client.subscribe(comandoMotor.c_str());
    }
    else
    {
      tentativa++;
      digitalWrite(LED_CONEXAO_MQTT, LOW);
      Serial.println("Tentando conexão MQTT");
      delay(500);
    }

    if (tentativa >= 5)
    {
      Serial.println("Conexão com o MQTT falhou");
      break;
    }
  }
}

//
// --- CONECTA AO WIFI ---
//
void ConexaoWifi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    piscaLed(LED_CONEXAO_WIFI);
  }

  Serial.println("WiFi conectado!");
}

//
// --- SETUP GERAL ---
//
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(LED_CAIXA_CHEIA, OUTPUT);
  pinMode(LED_CAIXA_METADE, OUTPUT);
  pinMode(LED_CAIXA_VAZIA, OUTPUT);
  pinMode(LED_CONEXAO_MQTT, OUTPUT);
  pinMode(LED_CONEXAO_WIFI, OUTPUT);
  pinMode(LED_COBEXAO_ARDUINO, OUTPUT);

  pinMode(SAIDA_MOTOR, OUTPUT);

  Serial.begin(115200);
  ConexaoWifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW); // RS485 em modo RECEBER

  rs485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(100);

  pinMode(BTN_MOTOR, INPUT_PULLUP);

  ConexaoWifi();
  Serial.println(WiFi.localIP());

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

//
// --- LOOP PRINCIPAL ---
//
void loop()
{
  ArduinoOTA.handle();

  // digitalWrite(LED_BUILTIN,HIGH);

  // Mantém conexão MQTT
  if (!client.connected() && tentativa <= 5)
  {
    reconnect();
  }
  client.loop();

  // Monitora comunicação RS485
  monitorarComunicacao();
  controleAltomaticoMotor(estadoSensorDeNivel1);

  // Lê dados da RS485
  String resultado = RecebimentoDeDados();
  if (resultado.length() > 0)
  {
    Serial.println(resultado);
    enviaEstadoDoSensor(resultado);
    // Atualiza LEDs da caixa
    nivelCaixa();
  }

  // LED piscando quando motor está ligado
  if (estadoBTN)
  {
    sinalisacaoMotor();
  }

  // Leitura do botão do motor
  bool leitura = digitalRead(BTN_MOTOR);

  // Borda de descida (aperto)
  if (leitura == LOW && ultimoEstadoBotao == HIGH)
  {
    estadoBTN = !estadoBTN; // Inverte
    digitalWrite(SAIDA_MOTOR, estadoBTN);

    client.publish(comandoMotor.c_str(), String(estadoBTN).c_str());
    client.publish(statusMotor.c_str(), String(estadoBTN).c_str());
  }
  delay(10);
  ultimoEstadoBotao = leitura;
}
