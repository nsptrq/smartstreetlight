#include <Wire.h>
#include <BH1750.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>
#include <esp_wifi.h>
BH1750 lightMeter;
Adafruit_INA219 ina219;




uint8_t broadcastAddress[] = {0x40, 0x4c, 0xca, 0x57, 0x5d, 0xa4};
  bool channelFound = false;




typedef struct pesansensor {
  float l;
  float arus;
  float tegangang;
  float daya;


} pesansensor;


pesansensor datasensor;
esp_now_peer_info_t peerInfo;


const char *WiFiSSID = "KOS BUBUTAN ATAS";


int32_t getWiFiChannel(const char *ssid) {
    if (int32_t n = WiFi.scanNetworks()) {
        for (uint8_t i=0; i<n; i++) {
            if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
                return WiFi.channel(i);
            }
        }
    }
    return 0;
}


uint8_t channel = 1;


void tryNextChannel() {
  Serial.println("Changing channel from " + String(channel) + " to " + String(channel+1));
  channel = channel % 13 + 1;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}




void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (!channelFound && status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Fail because channel" + String(channel) + " does not match receiver channel.");
    tryNextChannel(); // If message was not delivered, it tries on another wifi channel.
  }
  else {
    Serial.println("Delivery Successful ! Using channel : " + String(channel));
    channelFound = true;
  }
}

const int pirPin = 1;    // Pin sensor gerak PIR
const int ledPin = 2;     // Pin kontrol LED dengan PWM
const int pwmMax = 255;   // Nilai PWM maksimal
const int pwmHalf = 127;  // Nilai PWM setengah
const int pwmOff = 0;
unsigned long previousMillis = 0; // Penyimpanan waktu sebelumnya
const long interval = 5000; // Interval waktu 30 detik

void setup() {
  Serial.begin(115200);      // Inisialisasi serial monitor
  Wire.begin(6,7);           // Set pin SCL SDA BH1750
  WiFi.mode(WIFI_STA);

 if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1);  // Halt if INA219 is not detected
    }

  ina219.setCalibration_32V_2A();
    Serial.println("INA219 Initialized.");


  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      Serial.println(F("BH1750 configured and started successfully."));
  } else {
      Serial.println(F("Error: BH1750 not configured! Check wiring and I2C address."));
  }

  pinMode(pirPin, INPUT);    // Set pin PIR sebagai input
  pinMode(ledPin, OUTPUT);   // Set pin kontrol LED sebagai output

  // Set PWM awal ke setengah nilai
  analogWrite(ledPin, pwmHalf);
  Serial.println("Initial PWM set to Half");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  int32_t channel = getWiFiChannel(WiFiSSID);
esp_wifi_set_promiscuous(true);
esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
esp_wifi_set_promiscuous(false);
 
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
 
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }


}


void loop() {
  // Baca nilai sensor fotolistrik (cahaya sekitar)
  float lux = lightMeter.readLightLevel();


  // Kontrol relay berdasarkan nilai sensor fotolistrik (cahaya sekitar)
  if (lux < 20) { // Jika sensor fotolistrik mendeteksi cahaya terang
    analogWrite(ledPin, pwmMax); // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights ON");
  }
  else if(lux < 100 ){
    analogWrite(ledPin, pwmHalf); // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights ON Dimmed");
  }
  else if(lux >= 100) { // Jika sensor fotolistrik mendeteksi cahaya redup
    analogWrite(ledPin, pwmOff); // Matikan relay
    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.println(" - Lights OFF");
  }


  // Baca nilai sensor gerak PIR
  int pirValue = digitalRead(pirPin);
  unsigned long currentMillis = millis();


  if (pirValue == HIGH &&  lux < 20) { // Jika sensor PIR mendeteksi gerakan
    previousMillis = currentMillis; // Reset waktu
    analogWrite(ledPin, pwmMax); // Atur PWM ke nilai maksimal
    Serial.println("Motion detected - PWM Max");
  }


  // Hitung waktu yang tersisa sampai PWM turun setengahnya
  unsigned long timeLeft = (previousMillis + interval) - currentMillis;


  // Jika tidak ada gerakan selama lebih dari xx detik
  if (currentMillis - previousMillis >= interval && lux < 20) {
    analogWrite(ledPin, pwmHalf); // Atur PWM ke setengah nilai
    Serial.println("No motion - PWM Half");
  }
  else if (lux < 20) {
    // Tampilkan waktu yang tersisa pada serial monitor
    Serial.print("Time left: ");
    Serial.print(timeLeft / 1000); // Konversi milidetik ke detik
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

  datasensor.l = lux;
  datasensor.tegangang = busVoltage;
  datasensor.arus = current_mA;
  datasensor.daya = power_mW;
 
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &datasensor, sizeof(pesansensor));


if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  delay(100); // Tunggu 100 milidetik sebelum melakukan pengukuran ulang
}
