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

//
// --- VARIÁVEIS DE ESTADO ---
//
bool estadoBTN = false;        // Estado atual do botão (toggle)
bool ultimoEstadoBotao = HIGH; // Último estado lido do botão

//
// --- COMUNICAÇÃO RS485 ---
//
SoftwareSerial rs485Serial(RS485_RX, RS485_TX); // RX, TX

String RecebimentoDeDados(); // Protótipo da função de leitura

int quantidadeDeSesores = 3; // Quantidade total de sensores
String stadoDoSensorAnterior[] = {"1", "1", "1", "1"};
String stadoDoSensorAtual[] = {"0", "0", "0", "0"};

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

//
// --- FUNÇÃO: LIGA/DESLIGA MOTOR ---
//
String LigarMotor(int estadoRecebido)
{
  digitalWrite(L2, estadoRecebido);
  client.publish("stado/MOTOR", String(estadoRecebido).c_str());
  return String(estadoRecebido);
}

//
// --- VARIÁVEIS INICIAIS ---
//
int stadoledMotor = 0;
String Sensor1 = stadoDoSensorAtual[0];
String Sensor2 = stadoDoSensorAtual[1];
String Sensor3 = stadoDoSensorAtual[2];
String Motor = String(stadoledMotor);

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

  // Atualização inicial dos sensores e motor
  if (String(topic) == "init/atulisar")
  {
    if (message == "true")
    {
      client.publish("sensor/SEN[1]", Sensor1.c_str());
      client.publish("sensor/SEN[2]", Sensor2.c_str());
      client.publish("sensor/SEN[3]", Sensor3.c_str());
      client.publish("sensor/MOTOR", Motor.c_str());
      client.publish("stado/MOTOR", LigarMotor(estadoBTN).c_str());
    }
  }

  // Controle remoto do motor
  if (String(topic) == "s/sensor/MOTOR")
  {
    bool estadoRecebido = message.toInt();
    LigarMotor(estadoRecebido);
    estadoBTN = estadoRecebido;
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

      client.subscribe("init/atulisar");
      client.subscribe("s/sensor/MOTOR");
      client.subscribe("stado/MOTOR");
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
  rs485Serial.begin(9600);       // Deve coincidir com o baudrate do Arduino remoto
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
  char stado;

  if (resultado.length() > 0)
  {
    // Processa estado dos sensores
    for (int x = 0; x <= quantidadeDeSesores; x++)
    {
      if (resultado.startsWith("#SEN[" + String(x) + "]"))
      {
        stado = resultado.charAt(8);

        if (String(stado) != stadoDoSensorAnterior[x])
        {
          Serial.println("Sensor: " + resultado +
                         " | Estado Anterior: " + stadoDoSensorAnterior[x]);

          digitalWrite(leds[x], !stadoDoSensorAtual[x].toInt());

          String topic = "sensor/SEN[" + String(x) + "]";
          client.publish(topic.c_str(), stadoDoSensorAnterior[x].c_str());

          stadoDoSensorAtual[x] = stadoDoSensorAnterior[x];
          stadoDoSensorAnterior[x] = char(stado);
        }
      }
      stado = ' ';
    }
  }

  // --- LEITURA DO BOTÃO ---
  bool leitura = digitalRead(b1);

  // Detecta borda de descida (aperto do botão)
  if (leitura == LOW && ultimoEstadoBotao == HIGH)
  {
    estadoBTN = !estadoBTN; // Inverte o estado
    stadoledMotor = estadoBTN;
    LigarMotor(stadoledMotor);
    Motor = estadoBTN ? "1" : "0";

    client.publish("sensor/MOTOR", Motor.c_str());
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
