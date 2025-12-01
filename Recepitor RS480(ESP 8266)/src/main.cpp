#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//
// --- CONFIGURAÇÕES DE REDE WIFI ---
//
const char *ssid = "Ismailer Gregorio Oi Fibra 2.4G";
const char *password = "27270404";

//
// --- DEFINIÇÃO DE PINOS ---
//
#define RS485_RX D1   // GPIO4 — RO do módulo (dados recebidos)
#define RS485_TX D2   // GPIO5 — DI do módulo (não usado aqui)
#define RS485_CTRL D5 // GPIO14 — pino de controle (DE + RE juntos)

#define LED_CAIXA_VASIA_SERVIDOR D7 // LED que indica servidor offline
#define LED_CAIXA_CHEIA_WIFI D8     // LED que indica WiFi conectado
#define L2 D6                       // LED/Motor ligado ou desligado
#define b1 D0                       // Botão de controle manual do motor

// --- VARIAVEIS MQTT ----
//---variaveis de envio
String estadoCaixa = "estado/caixa";
String estadoSenorDeNivel1MQTT = "estado/nivel[1]";
String estadoSenorDeNivel2MQTT = "estado/nivel[2]";
String estadoSenorDeNivel3MQTT = "estado/nivel[3]";
String estadoBtnMotorDoEspMQTT = "estadoBtn/esp/motor";

//---variaveis de recebimento do site
String initSite = "init/atulisar";
String estadoBtnMotorDoSite = "estadoBtn/site/motor";
String estadoCaixaDoSite = "estado/site/caixa";

//
// --- VARIÁVEIS DE ESTADO ---
//

String nivelDaCaixa = "Vasio";
bool estadoSensorDeNivel1 = true;
bool estadoSensorDeNivel2 = true;
bool estadoSensorDeNivel3 = true;

bool estadoSensorDeNivel1Anterior = false;
bool estadoSensorDeNivel2Anterior = false;
bool estadoSensorDeNivel3Anterior = false;

bool estadoSaidaDoMotor = false;

bool estadoBTN = false;        // Estado atual do botão (toggle)
bool ultimoEstadoBotao = HIGH; // Último estado lido do botão

//
// --- COMUNICAÇÃO RS485 ---
//
SoftwareSerial rs485Serial(RS485_RX, RS485_TX); // RX, TX

String RecebimentoDeDados(); // Protótipo da função de leitura

// LEDs correspondentes aos sensores
int leds[] = {80, LED_CAIXA_CHEIA_WIFI, 80, LED_CAIXA_VASIA_SERVIDOR};

// Controle de tempo para piscagem de LEDs
unsigned long intervalo = 2000; // Intervalo de 2 segundos
unsigned long ultimoTempo = 0;  // Armazena o último tempo

//
// --- CONFIGURAÇÕES MQTT ---
//
const char *mqtt_server = "192.168.100.5"; // IP do broker
const int mqtt_port = 1883;
const char *mqtt_user = "meuuser";
const char *mqtt_pass = "1234";

WiFiClient espClient;
PubSubClient client(espClient);

//
// --- VARIÁVEIS DE CONTROLE DE LED ---
//
unsigned long ultimoTempo_F = 0; // Controle de tempo de pisca
bool estadoLed = LOW;

//
// --- FUNÇÃO: PISCA UM LED PERIODICAMENTE ---
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

//--- FUNÇÃO VERIFICA ESTADO DOS SENSORES ---
void enviaEstadoDoSensor(String resultado)
{
  String r = resultado.substring(0, 7);
  int v = resultado.substring(8).toInt();

  if (r == "#SEN[1]" && v != estadoSensorDeNivel1Anterior)
  {
    estadoSensorDeNivel1 = v;
    client.publish(String(estadoSenorDeNivel1MQTT).c_str(), String(v).c_str());
    Serial.println("#SEN[1]");
    // estadoSensorDeNivel1Anterior = estadoSensorDeNivel1;
  }
  else if (r == "#SEN[2]" && v != estadoSensorDeNivel2Anterior)
  {
    estadoSensorDeNivel2 = v;
    client.publish(String(estadoSenorDeNivel2MQTT).c_str(), String(v).c_str());
    // estadoSensorDeNivel2Anterior = estadoSensorDeNivel2;
  }
  else if (r == "#SEN[3]" && v != estadoSensorDeNivel3Anterior)
  {
    estadoSensorDeNivel3 = v;
    client.publish(String(estadoSenorDeNivel3MQTT).c_str(), String(v).c_str());
    // estadoSensorDeNivel3Anterior = estadoSensorDeNivel3;
  }
}
//--- FUNÇÃO VERIFICA NIVEL DE CAIXA ---
void nivelCaixa()
{

  if (estadoSensorDeNivel1 != estadoSensorDeNivel1Anterior || estadoSensorDeNivel2 != estadoSensorDeNivel2Anterior ||
      estadoSensorDeNivel3 != estadoSensorDeNivel3Anterior)
  {
    estadoSensorDeNivel1Anterior = estadoSensorDeNivel1;
    estadoSensorDeNivel2Anterior = estadoSensorDeNivel2;
    estadoSensorDeNivel3Anterior = estadoSensorDeNivel3;

    String estado = String(estadoSensorDeNivel1) + "," +
                    String(estadoSensorDeNivel2) + "," +
                    String(estadoSensorDeNivel3);
    if (estado == "0,1,1")
    {
      Serial.println("Vasio");
      nivelDaCaixa = "Vasio";
    }
    else if (estado == "0,0,1")
    {
      nivelDaCaixa = "Metade";
      Serial.println("Metade");
    }
    else if (estado == "0,0,0")
    {
      nivelDaCaixa = "Cheio";
      Serial.println("Cheio");
    }
    else
    {
      nivelDaCaixa = "Erro na Leitura";
      Serial.println("Erro na Leitura");
    }
    client.publish(String(estadoCaixa).c_str(), String(nivelDaCaixa).c_str());
  }
}

void comtroleDoMoto(String mensagem = "")
{
  String v = mensagem;
  Serial.println(v);
  if (mensagem == "1" || mensagem == "0")
  {
    digitalWrite(L2, v.toInt());
    estadoBTN = v.toInt();
    client.publish(String(estadoBtnMotorDoEspMQTT).c_str(), String(estadoBTN).c_str());
  }
  ultimoEstadoBotao = estadoBTN;
}
//

//
// --- CALLBACK: RECEBE MENSAGENS MQTT ---
//
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  // Converte o payload (byte array) em String
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  if (String(topic) == initSite && message == "true")
  {
    client.publish(String(estadoCaixa).c_str(), String(nivelDaCaixa).c_str());
    client.publish(String(estadoSenorDeNivel1MQTT).c_str(), String(estadoSensorDeNivel1).c_str());
    client.publish(String(estadoSenorDeNivel2MQTT).c_str(), String(estadoSensorDeNivel2).c_str());
    client.publish(String(estadoSenorDeNivel3MQTT).c_str(), String(estadoSensorDeNivel3).c_str());
    client.publish(String(estadoBtnMotorDoEspMQTT).c_str(), String(estadoBTN).c_str());
  }
  if (String(topic) == estadoBtnMotorDoSite)
  {
    comtroleDoMoto(message);
  }
  if (String(topic) == estadoCaixa)
  {
    client.publish(String(estadoCaixa).c_str(), String(nivelDaCaixa).c_str());
  }
}

//
// --- FUNÇÃO: TENTA RECONEXÃO MQTT ---
//
int tentativa = 0;

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Tentando conectar ao MQTT...");

    if (client.connect("ESP32Client", mqtt_user, mqtt_pass))
    {
      Serial.println(" Conectado!");
      digitalWrite(LED_CAIXA_VASIA_SERVIDOR, LOW);

      client.subscribe(initSite.c_str());
      client.subscribe(estadoBtnMotorDoSite.c_str());
    }
    else
    {
      Serial.print(" Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 1s...");

      tentativa++;
      digitalWrite(LED_CAIXA_VASIA_SERVIDOR, HIGH);
      delay(500);
    }

    if (tentativa >= 5)
    {
      break;
    }
  }
}

//
// --- FUNÇÃO: CONEXÃO COM WIFI ---
//
void ConexaoWifi()
{
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    piscaLed(LED_CAIXA_CHEIA_WIFI);
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//
// --- SETUP: INICIALIZA O SISTEMA ---
//
void setup()
{
  pinMode(LED_CAIXA_VASIA_SERVIDOR, OUTPUT);
  pinMode(LED_CAIXA_CHEIA_WIFI, OUTPUT);
  pinMode(L2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  ConexaoWifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW); // Inicialmente em recepção

  rs485Serial.begin(9600); // Deve coincidir com o baudrate do Arduino remoto
  delay(100);

  digitalWrite(LED_CAIXA_VASIA_SERVIDOR, LOW);
  digitalWrite(LED_CAIXA_CHEIA_WIFI, LOW);

  pinMode(b1, INPUT_PULLUP);

  Serial.println("Receptor Ativo!");
}

//
// --- LOOP PRINCIPAL ---
//
void loop()
{
  // Mantém conexão com o broker MQTT
  if (!client.connected() && tentativa <= 5)
  {
    reconnect();
  }
  client.loop();

  // Lê dados recebidos via RS485
  String resultado = RecebimentoDeDados();
  if (resultado.length() > 0)
  {
    // Serial.println("Entro");
    enviaEstadoDoSensor(resultado);
    nivelCaixa();
  }

  // --- LEITURA DO BOTÃO ---
  bool leitura = digitalRead(b1);

  // Detecta borda de descida (aperto do botão)
  if (leitura == LOW && ultimoEstadoBotao == HIGH)
  {
    estadoBTN = !estadoBTN; // Inverte o estado

    digitalWrite(L2, estadoBTN);
    client.publish(String(estadoBtnMotorDoEspMQTT).c_str(), String(estadoBTN).c_str());
  }

  ultimoEstadoBotao = leitura; // Atualiza o último estado do botão
}

//
// --- FUNÇÃO: RECEBE DADOS VIA RS485 ---
//
String RecebimentoDeDados()
{
  String dados = "";

  if (rs485Serial.available())
  {
    dados = rs485Serial.readStringUntil('\n'); // Lê até o caractere '\n'
  }

  return dados;
}
