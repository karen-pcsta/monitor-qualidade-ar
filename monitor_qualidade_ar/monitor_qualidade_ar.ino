// Monitor de qualidade do ar - NodeMCU + DHT22 + MQ-135 + LEDs + ThingSpeak (MQTT)

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===== CREDENCIAIS =====
// Copiar secrets.example.h para secrets.h (mesma pasta) e preencher os dados.
// O secrets.h esta no .gitignore e nao vai para o repositorio.
#include "secrets.h"
// =======================

#define LED_VERDE    D2
#define LED_VERMELHO D5
#define LED_AMARELO  D6

DHT dht(D1, DHT22);
WiFiClient wifi;
PubSubClient mqtt(wifi);

float referencia;                 // calibrar leitura do MQ-135 no ar do ambiente
float temperatura = 0, umidade = 0;
int ciclo = 0;

// Media de 10 leituras do MQ-135 para reduzir ruido
int lerGas() {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(A0);
    delay(10);
  }
  return soma / 10;
}

// 0 = adequado, 1 = atencao, 2 = critico
int nivel(float valor, float minimo, float maximo, float margem) {
  if (valor >= minimo && valor <= maximo) return 0;
  if (valor >= minimo - margem && valor <= maximo + margem) return 1;
  return 2;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_NOME, WIFI_SENHA);             // conecta em segundo plano
  mqtt.setServer("mqtt3.thingspeak.com", 1883);

  // Pre-aquecimento: amarelo aceso, sem alertas
  Serial.println("Aquecendo o MQ-135 (2 min)...");
  digitalWrite(LED_AMARELO, HIGH);
  delay(120000);

  // Calibracao automatica: media do ar do ambiente por 1 min
  Serial.println("Calibrando (1 min)...");
  long soma = 0;
  for (int i = 0; i < 60; i++) {
    soma += lerGas();
    delay(1000);
  }
  referencia = soma / 60.0;
  digitalWrite(LED_AMARELO, LOW);
  Serial.print("Referencia: ");
  Serial.println(referencia);
}

void loop() {
  // 1. Leitura dos sensores (se o DHT22 falhar, mantem o valor anterior)
  float t = dht.readTemperature();
  float u = dht.readHumidity();
  if (!isnan(t) && !isnan(u)) {
    temperatura = t;
    umidade = u;
  }
  int gas = lerGas();

  // 2. Avaliacao: L1 = 20% e L2 = 50% acima da referencia
  int nivelGas = 0;
  if (gas > referencia * 1.5) nivelGas = 2;
  else if (gas > referencia * 1.2) nivelGas = 1;
  int pior = max(nivelGas, max(nivel(temperatura, 21, 26, 3), nivel(umidade, 35, 65, 10)));

  // 3. Atuador: acende so o LED do pior nivel
  digitalWrite(LED_VERDE,    pior == 0);
  digitalWrite(LED_AMARELO,  pior == 1);
  digitalWrite(LED_VERMELHO, pior == 2);

  Serial.printf("Temp: %.1f C | Umid: %.1f %% | Gas: %d | Nivel: %d\n",
                temperatura, umidade, gas, pior);

  // 4. Envio ao ThingSpeak a cada 20 s (10 ciclos de 2 s)
  ciclo++;
  if (ciclo >= 10) {
    ciclo = 0;
    if (WiFi.status() == WL_CONNECTED &&
        (mqtt.connected() || mqtt.connect(MQTT_CLIENT_ID, MQTT_USUARIO, MQTT_SENHA))) {
      String msg = "field1=" + String(temperatura, 1) +
                   "&field2=" + String(umidade, 1) +
                   "&field3=" + String(gas);
      mqtt.publish(TOPICO, msg.c_str());
      Serial.println("Publicado: " + msg);
    } else {
      Serial.println("Sem conexao: envio ignorado neste ciclo");
    }
  }

  mqtt.loop();
  delay(2000);
}
