#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>
#include <PubSubClient.h>
#include <esp_wifi.h>

Adafruit_INA219 ina219;

//ssid dan password untuk wifi
const char* ssid = "Kos kurma";
const char* password = "berkahselalu";

// MQTT broker details
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

typedef struct pesansensor {
  float l;
  float arus;
  float tegangan;
  float daya;
} pesansensor;

pesansensor datasensor;
esp_now_peer_info_t peerInfo;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  WiFi.printDiag(Serial);
esp_wifi_set_promiscuous(true);
esp_wifi_set_channel(8, WIFI_SECOND_CHAN_NONE);
esp_wifi_set_promiscuous(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    if (client.connect("ESPClient")) {
      Serial.println("Connected");
      client.subscribe("faiqnicky/cahaya");
      client.subscribe("faiqnicky/tegangan1");
      client.subscribe("faiqnicky/daya1");
      client.subscribe("faiqnicky/arus1");
      client.subscribe("faiqnicky/tegangan2");
      client.subscribe("faiqnicky/daya2");
      client.subscribe("faiqnicky/arus2");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void OnDataRecv(const esp_now_recv_info_t* mac_addr, const uint8_t* incomingData, int len) {
  memcpy(&datasensor, incomingData, sizeof(datasensor));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Lux: ");
  Serial.println(datasensor.l);
}

const int pirPin = 1;     // Pin sensor gerak PIR
const int ledPin = 2;     // Pin kontrol LED dengan PWM
const int pwmMax = 255;   // Nilai PWM maksimal
const int pwmHalf = 127;  // Nilai PWM setengah
const int pwmOff = 0;
unsigned long previousMillis = 0;  // Penyimpanan waktu sebelumnya
const long interval = 5000;        // Interval waktu 30 detik

void setup() {
  Serial.begin(115200);  // Inisialisasi serial monitor
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Wire.begin(6, 7);  // Set pin SCL SDA BH1750

  WiFi.mode(WIFI_STA);

  if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1);  // Halt if INA219 is not detected
    }

    ina219.setCalibration_32V_2A();
    Serial.println("INA219 Initialized.");

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  pinMode(pirPin, INPUT);   // Set pin PIR sebagai input
  pinMode(ledPin, OUTPUT);  // Set pin kontrol LED sebagai output

  // Set PWM awal ke setengah nilai
  analogWrite(ledPin, pwmHalf);
  Serial.println("Initial PWM set to Half");
}

void loop() {
  // Baca nilai sensor fotolistrik (cahaya sekitar)

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float lux = datasensor.l;
  // Kontrol relay berdasarkan nilai sensor fotolistrik (cahaya sekitar)
  if (lux < 20) {                 // Jika sensor fotolistrik mendeteksi cahaya terang
    analogWrite(ledPin, pwmMax);  // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights ON");
  } else if (lux < 100) {
    analogWrite(ledPin, pwmHalf);  // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights ON Dimmed");
  } else if (lux >= 100) {        // Jika sensor fotolistrik mendeteksi cahaya redup
    analogWrite(ledPin, pwmOff);  // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights OFF");
  }

  // Baca nilai sensor gerak PIR
  int pirValue = digitalRead(pirPin);
  unsigned long currentMillis = millis();


  if (pirValue == HIGH && lux < 20) {  // Jika sensor PIR mendeteksi gerakan
    previousMillis = currentMillis;    // Reset waktu
    analogWrite(ledPin, pwmMax);       // Atur PWM ke nilai maksimal
    Serial.println("Motion detected - PWM Max");
  }

  // Hitung waktu yang tersisa sampai PWM turun setengahnya
  unsigned long timeLeft = (previousMillis + interval) - currentMillis;

  // Jika tidak ada gerakan selama lebih dari xx detik
  if (currentMillis - previousMillis >= interval && lux < 20) {
    analogWrite(ledPin, pwmHalf);  // Atur PWM ke setengah nilai
    Serial.println("No motion - PWM Half");
  } else if (lux < 20) {
    // Tampilkan waktu yang tersisa pada serial monitor
    Serial.print("Time left: ");
    Serial.print(timeLeft / 1000);  // Konversi milidetik ke detik
    Serial.println(" seconds");
  }


  // Indikasi status PWM sebelum dan sesudah mendeteksi gerakan
  Serial.print("Current PWM: ");


  if (pirValue == HIGH) {
    Serial.println(pwmMax);
  } else if (currentMillis - previousMillis >= interval) {
    Serial.println(pwmHalf);
  } else {
    Serial.println(pwmMax);
  }

  float busVoltage = ina219.getBusVoltage_V();        // Read bus voltage
    float current_mA = ina219.getCurrent_mA();          // Read current in mA
    float power_mW = busVoltage * current_mA;
    // Print readings to the Serial Monitor
    Serial.print("Bus Voltage:   "); Serial.print(busVoltage); Serial.println(" V");
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:       "); Serial.print(power_mW); Serial.println(" mW");
    Serial.println("--------------------------------------------");

  char cahaya[8];
  char tegangan1[8];
  char arus1[8];
  char daya1[8];
  char tegangan2[8];
  char arus2[8];
  char daya2[8];

  dtostrf(lux, 6, 2, cahaya);
  dtostrf(datasensor.tegangan, 6, 2, tegangan1);
  dtostrf(datasensor.arus, 6, 2, arus1);
  dtostrf(datasensor.daya, 6, 2, daya1);
  dtostrf(busVoltage, 6, 2, tegangan2);
  dtostrf(current_mA, 6, 2, arus2);
  dtostrf(power_mW, 6, 2, daya2);


  client.publish("faiqnicky/cahaya", cahaya, false);
  client.publish("faiqnicky/tegangan1", tegangan1, false);
  client.publish("faiqnicky/arus1", arus1, false);
  client.publish("faiqnicky/daya1", daya1, false);
  client.publish("faiqnicky/tegangan2", tegangan2, false);
  client.publish("faiqnicky/arus2", arus2, false);
  client.publish("faiqnicky/daya2", daya2, false);

  delay(100);  // Tunggu 100 milidetik sebelum melakukan pengukuran ulang
}
