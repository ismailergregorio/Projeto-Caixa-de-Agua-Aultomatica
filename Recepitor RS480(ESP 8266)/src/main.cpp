#include <Arduino.h>

#include <SoftwareSerial.h>

#define RS485_RX D2   // GPIO4 — RO do módulo (dados recebidos)
#define RS485_TX D1   // GPIO5 — DI do módulo (não usado aqui)
#define RS485_CTRL D5 // GPIO14 — DE + RE juntos

SoftwareSerial rs485Serial(RS485_RX, RS485_TX); // RX, TX

void RecebimentoDeDados();

String S1;
String S2;
String S3;

String dados;

void setup()
{
  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, LOW); // Inicialmente em recepção
  Serial.begin(115200);          // Para debug

  rs485Serial.begin(9600); // Deve bater com o ESP

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Recpitor Ativo!");
}

void loop()
{
  RecebimentoDeDados();
}

void RecebimentoDeDados()
{
  if (rs485Serial.available())
  {
    String dados = rs485Serial.readStringUntil('\n'); // Lê até o caractere '\n'

    Serial.println("Dados Recebidos:");
    Serial.println(dados);

    // if (dados.startsWith("#SEN[1]:"))
    // {
    //   String valor = dados.substring(8);
    //   Serial.println("dado recebido e do SEN[1] e " + valor);
    //   S1 = valor;
    //   int valorS1 = S1.toInt();
    //   digitalWrite(LED_BUILTIN, valorS1);
    // }
    // else if (dados.startsWith("#SEN[2]:"))
    // {
    //   String valor = dados.substring(8);

    //   Serial.println("dado recebido e do SEN[2] e " + valor);
    //   S2 = valor;
    // }
    // else if (dados.startsWith("#SEN[3]:"))
    // {
    //   String valor = dados.substring(8);

    //   Serial.println("dado recebido e do SEN[3] e " + valor);
    //   S3 = valor;
    // }
    // else
    // {
    //   Serial.println("Comando desconhecido");
    // }
  }
}