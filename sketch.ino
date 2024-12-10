#include<WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <FS.h>
#include "SPIFFS.h"

#define IO_USERNAME  "deborahmacedo"
#define IO_KEY       "aio_BZgp74A3ZrsdgtHsSQbWvJdUbMPU"
const char* ssid = "Wokwi-GUEST"; //no arduino mudar para a rede local
const char* password =""; //senha da rede local

const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


#define PIN_TRIG1 13
#define PIN_ECHO1 14
#define PIN_TRIG2 26
#define PIN_ECHO2 18
#define LED1 32

#include <DHTesp.h>

const int DHT_PIN = 15;

DHTesp dhtSensor;

//Variáveis SPPIFS
String strContadorEntrada, readSContadorEntrada;
String strContadorSaida, readSContadorSaida;

enum Estado {
  E1_0_E2_0,
  E1_1_E2_0,
  E1_1_E2_1,
  E1_0_E2_1
};

Estado estadoAtual = E1_0_E2_0;
int contadorEntrada = 0;
int contadorSaida = 0;
int contador = 0;

//Ar
float arFalso = 22;
int oldContador = 0;

//inicio do spiffs


void writeFile(String state, String path) {
  File rFile = SPIFFS.open(path, "w");
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo para escrita!");
    return;
  }
  if (rFile.print(state)) {
    Serial.print("Gravou: ");
    Serial.println(state);
  } else {
    Serial.println("Erro ao gravar no arquivo!");
  }
  rFile.close();
}

String readFile(String path) { // Função que lê o código já gravado no ESP no endereço especificado
  File rFile = SPIFFS.open(path, "r");
  if (rFile) {
    String s = rFile.readStringUntil('\n');
    s.trim();
    Serial.print("Lido: ");
    Serial.println(s);
    rFile.close();
    return s;
  } else {
    Serial.println("Erro ao abrir arquivo!");
    return "";
  }
}

void openFS() { // Função que inicia o protocolo
  if (SPIFFS.begin()) {
    Serial.println("Sistema de arquivos aberto com sucesso!");
  } else {
    Serial.println("Erro ao abrir o sistema de arquivos");
  }
}
//---------------------------------FINAL SPPIFS----------------------------------



//-------------------------INICIO CONEXÃO WIFI Adafuit.io------------------------
void setup_wifi() {

  delay(10);
 
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Messagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    
  }
  Serial.println();

  

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Create a random client ID
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);
    // Se conectado
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      // Depois de conectado, publique um anúncio ...
      client.publish("deborahmacedo/feeds/Temperatura do AC", "Iniciando Comunicação");

      //... e subscribe. // <<<<----- mudar aqui
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}
//-------------------------FINAL CONEXÃO WIFI Adafuit.io-------------------------



//---------------------------IMPLEMENTAÇÃO DA DISTÂNCIA--------------------------
int medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  int duration = pulseIn(echoPin, HIGH, 20000);
  if (duration == 0) return 999;
  int cm = duration / 58;
  return cm;
}

void atualizarEstado(int E1, int E2) {
  switch (estadoAtual) {
    case E1_0_E2_0:
      if (E1 == 1 && E2 == 0) {
        estadoAtual = E1_1_E2_0;
      }
      break;

    case E1_1_E2_0:
      if (E1 == 1 && E2 == 1) {
        estadoAtual = E1_1_E2_1;
      } else if (E1 == 0 && E2 == 0) {
        contadorSaida--;
        estadoAtual = E1_0_E2_0;
        digitalWrite(LED1, HIGH);
        delay(500);
        digitalWrite(LED1, LOW);
        
      }
      break;

    case E1_1_E2_1:
      if (E1 == 0 && E2 == 1) {
        estadoAtual = E1_0_E2_1;
      } else if (E1 == 0 && E2 == 0) {
        estadoAtual = E1_0_E2_0;
      }
      break;

    case E1_0_E2_1:
      if (E1 == 0 && E2 == 0) {
        contadorEntrada++;
        estadoAtual = E1_0_E2_0;
        digitalWrite(LED1, HIGH);
        delay(500);
        digitalWrite(LED1, LOW);
        
      }
      break;

    default:
      estadoAtual = E1_0_E2_0;
      break;
 
  }
}
//--------------------------IMPLEMENTAÇÃO DA DISTÂNCIA---------------------------



void setup() {
  
  setup_wifi();
  client.setServer(mqttserver, 1883); // Publicar
  client.setCallback(callback); // Receber mensagem
  openFS();
  
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(PIN_TRIG1, OUTPUT);
  pinMode(PIN_ECHO1, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(PIN_TRIG2, OUTPUT);
  pinMode(PIN_ECHO2, INPUT);

  //ENTRADA
  if (readFile("/logsContadorEntrada.txt") != "") {
    readSContadorEntrada = readFile("/logsContadorEntrada.txt");
    contadorEntrada = readSContadorEntrada.toInt();

    if (contadorEntrada == 0 && readSContadorEntrada != "0") { // Validação adicional
      contadorEntrada = 0;
    }

  } else {
    contadorEntrada = 0;
    
  }

  //SAIDA
  if (readFile("/logsContadorSaida.txt") != "") {
    readSContadorSaida = readFile("/logsContadorSaida.txt");
    contadorSaida = readSContadorSaida.toInt(); // Converte para inteiro

    if (contadorSaida == 0 && readSContadorSaida != "0") { // Validação adicional
      contadorSaida = 0;
    }

  } else {
    contadorSaida = 0;
    
  }

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  int distancia1 = medirDistancia(PIN_TRIG1, PIN_ECHO1);
  int distancia2 = medirDistancia(PIN_TRIG2, PIN_ECHO2);
  int E1 = (distancia1 < 5) ? 1 : 0;
  int E2 = (distancia2 < 5) ? 1 : 0;

  atualizarEstado(E1, E2);

  contador = contadorEntrada + contadorSaida;

  if (oldContador != contador){

    if(oldContador > contador && arFalso > 20){
      arFalso = arFalso + 0.2;

    }
    if(oldContador < contador && arFalso < 24){
      arFalso = arFalso - 0.2;

    }

    //SPPIFS
    strContadorEntrada = String(contadorEntrada);
    writeFile(strContadorEntrada,"/logsContadorEntrada.txt");

    strContadorSaida = String(contadorSaida);
    writeFile(strContadorSaida, "/logsContadorSaida.txt");

    oldContador = contador;
  }

  //AR SIMULADO / CONTADOR GERAL
  Serial.println(" - Ar falso: " + String(arFalso) + " | Contador de Entradas e Saídas: " + String(contador));
  
  //DISTÂNCIA
  Serial.println(" - Distancia Sensor 1 (E1): " + String(distancia1) + " cm, E1: " + String(E1) + " | Distancia Sensor 2 (E2): " + String(distancia2) + " cm, E2: " + String(E2));

  //ENTRADAS
  Serial.println(" - Contador de Entradas: " + String(contadorEntrada) + " | Contador de Saídas: " + String(contadorSaida));

  //TEMPERATURA
  //Serial.println(" - Temp: " + String(data.temperature) + "°C");

  Serial.println("---");
  char s_temp[8];
    dtostrf(arFalso,1,2,s_temp);
    Serial.print("Temperatura: ");
    Serial.println(s_temp);
    client.publish("deborahmacedo/feeds/Temperatura do AC", s_temp);
    
  
  delay(100);
}
