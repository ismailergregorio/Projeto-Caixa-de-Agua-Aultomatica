#include <Arduino.h>
#include <SoftwareSerial.h>

// ==== Configuração de Pinos ====
#define RS485_TX 10  // D10 → DI do módulo RS485
#define RS485_RX 11  // D11 → RO do módulo RS485
#define RS485_CTRL 2 // D2  → DE + RE juntos

SoftwareSerial rs485Serial(RS485_RX, RS485_TX); // RX, TX

String dados;

// Sensores
#define PORTA_S1 7 // SEN[1]
#define PORTA_S2 8 // SEN[2]
#define PORTA_S3 9 // SEN[3]

// Códigos dos sensores
#define CODIFDO_SENSOR_1 "#SEN[1]:"
#define CODIFDO_SENSOR_2 "#SEN[2]:"
#define CODIFDO_SENSOR_3 "#SEN[3]:"

String S1 = "0";
String S2 = "0";
String S3 = "0";

// LEDs (pinos diferentes dos sensores para evitar conflito)
#define LED1 3
#define LED2 4

unsigned long millisInicial;
unsigned long millisAxiliar = 0;
int tempoInit = 1000; // intervalo para imprimir "."

bool conectado = false;

void EnvioDeDados();
void ControleDeLed(int pinSensor,int pinLed);

void setup()
{
  pinMode(RS485_CTRL, OUTPUT);
  digitalWrite(RS485_CTRL, HIGH); // Modo recepção

  pinMode(PORTA_S1, INPUT_PULLUP);
  pinMode(PORTA_S2, INPUT_PULLUP);
  pinMode(PORTA_S3, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  digitalWrite(LED1,LOW);
  digitalWrite(LED1,HIGH);

  Serial.begin(9600);      // Serial para debug
  rs485Serial.begin(9600); // RS485 mesma taxa

  Serial.println("Arduino Nano pronto para envio");

  millisInicial = millis(); // inicializa após setup
  // Init();
}

void loop()
{ 
  ControleDeLed(PORTA_S1,LED1);
  ControleDeLed(PORTA_S3,LED2);
  EnvioDeDados();
  delay(1000); // intervalo entre envios
}

void EnvioDeDados()
{
  digitalWrite(RS485_CTRL, HIGH); // Modo transmissão
  delay(5);                       // tempo para estabilizar

  int stadoSensor1 = digitalRead(PORTA_S1);
  int stadoSensor2 = digitalRead(PORTA_S2);
  int stadoSensor3 = digitalRead(PORTA_S3);

  String dado_sensor1 = CODIFDO_SENSOR_1 + String(stadoSensor1);
  String dado_sensor2 = CODIFDO_SENSOR_2 + String(stadoSensor2);
  String dado_sensor3 = CODIFDO_SENSOR_3 + String(stadoSensor3);

  String dadosParaEnvio = dado_sensor1 + "\n" +
                          dado_sensor2 + "\n" +
                          dado_sensor3 + "\n";

  rs485Serial.print(dadosParaEnvio);
  Serial.println(dadosParaEnvio);

  delay(5);                      // tempo para garantir envio
  digitalWrite(RS485_CTRL, LOW); // Modo recepção
}
void ControleDeLed(int pinSensor,int pinLed){
  int stadoSenso = digitalRead(pinSensor);
  // Serial.print(pinSensor);

  if(!stadoSenso){
    digitalWrite(pinLed,HIGH);
  }else{
    digitalWrite(pinLed,LOW);
  }
}
