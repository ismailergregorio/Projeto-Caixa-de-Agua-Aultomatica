#include <Arduino.h>

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid = "Ismailer Gregorio Oi Fibra 2.4G";
const char *password = "27270404";

#define RS485_RX D1   // GPIO4 — RO do módulo (dados recebidos)
#define RS485_TX D2   // GPIO5 — DI do módulo (não usado aqui)
#define RS485_CTRL D5 // GPIO14 — DE + RE juntos

#define LED_CAIXA_VASIA_SERVIDOR D7 // LED CAIXA VASIA
#define LED_CAIXA_CHEIA_WIFI D8     // LED CAIXA CHEIA

#define L2 D6 // LED MOTOR DESLIGADO/LIGADO

#define b1 D0

SoftwareSerial rs485Serial(RS485_RX, RS485_TX); // RX, TX

String RecebimentoDeDados();

int quantidadeDeSesores = 3;
String stadoDoSensorAnterior[] = {"0", "0", "0", "0"};
String stadoDoSensorAtual[] = {"1", "1", "1", "1"};

int leds[] = {80,LED_CAIXA_CHEIA_WIFI, 80,LED_CAIXA_VASIA_SERVIDOR};

unsigned long intervalo = 2000; // 1 segundo
unsigned long ultimoTempo = 0;  // Armazena último tempo que o LED mudou
                                // String dados;
// Configurações do MQTT
const char *mqtt_server = "192.168.100.5"; // IP do broker
const int mqtt_port = 1883;
const char *mqtt_user = "meuuser"; // usuário criado no Mosquitto
const char *mqtt_pass = "1234";    // senha

WiFiClient espClient;           // cliente WiFi
PubSubClient client(espClient); // cliente MQTT

unsigned long ultimoTempo_F = 0; // Armazena o último tempo que o LED mudou
bool estadoLed = LOW;            // Estado atual do LED

void piscaLed(int led)
{
  unsigned long tF = millis(); // Atualiza o tempo sempre

  if (tF - ultimoTempo_F >= 250)
  {
    // Alterna o estado do LED
    estadoLed = !estadoLed;
    digitalWrite(led, estadoLed);

    // Atualiza o último tempo
    ultimoTempo_F = tF;
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String r = "";
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++)
  {
    r = r + (char)payload[i];
    // Serial.println(r);
  }
  if (r == "teste")
  {
    digitalWrite(L2, HIGH);
  }
  // Serial.println(r);
}

String Sensor1;
String Sensor2;
String Sensor3;
String Motor;

void inity()
{
  client.publish("sensor/SEN[1]", Sensor1.c_str());
  client.publish("sensor/SEN[2]", Sensor2.c_str());
  client.publish("sensor/SEN[3]", Sensor3.c_str());
  client.publish("sensor/MOTOR", Motor.c_str());
}

int tentativa = 0;
void reconnect()
{
  // Loop até reconectar
  while (!client.connected())
  {
    Serial.print("Tentando conectar MQTT...");
    // clientID pode ser qualquer nome único
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass))
    {
      Serial.println("conectado!");
      digitalWrite(LED_CAIXA_VASIA_SERVIDOR, LOW);
      client.subscribe("teste/topico");
      client.subscribe("sensor/SEN[1]");
      client.subscribe("sensor/SEN[2]");
      client.subscribe("sensor/SEN[3]");
      client.subscribe("sensor/MOTOR");
    }
    else
    {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando de novo em 1s");
      Serial.println("Tentativa:");
      Serial.println(tentativa);
      tentativa = tentativa + 1;
      digitalWrite(LED_CAIXA_VASIA_SERVIDOR, HIGH);
      delay(500);
    }
    if (tentativa >= 5)
    {
      break;
    }
  }
}

void ConexaoWifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    piscaLed(LED_CAIXA_CHEIA_WIFI);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
bool estadoBTN = false;        // Estado do botão (controle de toggle)
bool ultimoEstadoBotao = HIGH; // Último estado lido do botão
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
  rs485Serial.begin(9600);       // Deve bater com o arduino nano
  delay(100);

  digitalWrite(LED_CAIXA_VASIA_SERVIDOR, LOW);
  digitalWrite(LED_CAIXA_CHEIA_WIFI, LOW);

  pinMode(b1, INPUT_PULLUP);

  Serial.println("Recpitor Ativo!");
}

void loop()
{
  if (!client.connected() && tentativa <= 5)
  {
    reconnect();
  }
  client.loop();

  String resultado;
  resultado = RecebimentoDeDados();
  char stado;
  if (resultado.length() > 0)
  {
    for (int x = 0; x <= quantidadeDeSesores; x++)
    {
      if (resultado.startsWith("#SEN[" + String(x) + "]"))
      {
        stado = resultado.charAt(8);
        if (String(stado) != stadoDoSensorAnterior[x])
        {
          Serial.println("sensor:" + resultado + ":  estado Anterior: " + stadoDoSensorAnterior[x]);
          Serial.println(stadoDoSensorAnterior[x]);
          digitalWrite(leds[x],!stadoDoSensorAtual[x].toInt());

          String s = "sensor/SEN["+ String(x) + "]";

          String st = String(stado);

          client.publish(s.c_str(), st.c_str());

          stadoDoSensorAtual[x] = stadoDoSensorAnterior[x];
          stadoDoSensorAnterior[x] = char(stado);
        }
      }
      stado = ' ';
    }
  }

  bool leitura = digitalRead(b1);
  // Detecta borda de descida (quando aperta)
  if (leitura == LOW && ultimoEstadoBotao == HIGH)
  {
    estadoBTN = !estadoBTN;      // Inverte o estado
    digitalWrite(L2, estadoBTN); // Atualiza saída
    Motor = estadoBTN ? "1" : "0";
  }
  ultimoEstadoBotao = leitura; // Atualiza estado anterior
}

String RecebimentoDeDados()
{

  String dados;

  if (rs485Serial.available())
  {
    dados = rs485Serial.readStringUntil('\n'); // Lê até o caractere '\n'
  }
  return dados;
}