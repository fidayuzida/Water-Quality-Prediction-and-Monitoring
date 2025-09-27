#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoOTA.h>

// === WIFI DAN FIREBASE ===
#define WIFI_SSID "Galaxy M12D95F"
#define WIFI_PASSWORD "zxdh6690"
#define API_KEY "AIzaSyBdQOzx9TmyFHUVCxVgJM_w6ZwgxsTfLBs"
#define DATABASE_URL "https://rf-bioflok-default-rtdb.asia-southeast1.firebasedatabase.app/"

// === PIN SENSOR ===
#define PH_SENSOR_PIN 36
#define TURBIDITY_SENSOR_PIN 34
#define TEMP_SENSOR_PIN 5

// === INISIALISASI FIREBASE & SENSOR ===
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);

// === LCD I2C ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === VARIABEL SENSOR & INTERVAL ===
float pHSum = 0.0, turbiditySum = 0.0, tempSum = 0.0;
int readingCount = 0;
unsigned long dataSendPreviousMillis = 0;
const long dataSendInterval = 180000; // 2 menit

// === VARIABEL LCD TAMPILAN ===
bool shouldDisplayLCD = false;
unsigned long lcdScrollPreviousMillis = 0;
const long lcdScrollInterval = 3000;
int lcdDisplayState = 0;
float lastPH = 0, lastNTU = 0, lastTemp = 0;

// === OTA via Arduino IDE ===
ArduinoOTAClass OTA;

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    Serial.printf("TOKEN ERROR: %s\n", info.error.message.c_str());
  } else {
    Serial.println("Token generation successful.");
  }
}

String getCurrentTimestamp() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  time_t epochTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&epochTime);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeInfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- Bioflok Monitoring System Initializing ---");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 20) {
    Serial.print(".");
    delay(500);
    wifiRetries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERROR] WiFi Failed to Connect. Restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while (!timeClient.update()) timeClient.forceUpdate();

  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    Serial.println("No temperature sensor found!");
    delay(3000);
    ESP.restart();
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (!Firebase.signUp(&config, &auth, "", "")) {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
    delay(3000);
    ESP.restart();
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  delay(2000);  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring...");


  ArduinoOTA.setHostname("esp32-bioflok");
  ArduinoOTA.begin();

  Serial.println("\n--- Setup complete. Starting main loop. ---\n");
}

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(1000);
  }

  sensors.requestTemperatures();
  float currentTemp = sensors.getTempCByIndex(0);
  if (currentTemp != -127.0) tempSum += currentTemp;

  float pHVoltageSum = 0, turbidityVoltageSum = 0;
  for (int i = 0; i < 10; i++) {
    pHVoltageSum += ((float)analogRead(PH_SENSOR_PIN) / 4095.0) * 3.3;
    turbidityVoltageSum += ((float)analogRead(TURBIDITY_SENSOR_PIN) / 4095.0) * 3.3;
    delay(10);
  }

  float avgPHVoltage = pHVoltageSum / 10;
  float currentPH = 35.8982 * exp(-avgPHVoltage / 1.1596);
  currentPH = constrain(currentPH, 0.0, 14.0);
  pHSum += currentPH;

  float avgTurbidityVoltage = turbidityVoltageSum / 10;
  float currentNtu = (avgTurbidityVoltage < 1.65) ? 3000 :
                     (avgTurbidityVoltage > 2.77) ? 0 :
                     -4352.94 + 8700.45 * avgTurbidityVoltage - 2572.20 * pow(avgTurbidityVoltage, 2);
  currentNtu = constrain(currentNtu, 0, 3000);
  turbiditySum += currentNtu;

  readingCount++;

  if (millis() - dataSendPreviousMillis >= dataSendInterval) {
    if (Firebase.ready() && readingCount > 0) {
      float avgPH = pHSum / readingCount;
      float avgTurbidity = turbiditySum / readingCount;
      float avgTemperature = tempSum / readingCount;

      FirebaseJson json;
      json.set("pH", avgPH);
      json.set("turbidity", avgTurbidity);
      json.set("temperature", avgTemperature);
      json.set("timestamp", getCurrentTimestamp());

      String path = "sensor_readings/" + String(timeClient.getEpochTime());
      if (!Firebase.setJSON(fbdo, path, json)) {
        Serial.printf("Firebase Error: %s\n", fbdo.errorReason().c_str());
        ESP.restart();
      }

      lastPH = avgPH;
      lastNTU = avgTurbidity;
      lastTemp = avgTemperature;

      pHSum = turbiditySum = tempSum = 0.0;
      readingCount = 0;
      shouldDisplayLCD = true;
      lcdDisplayState = 0;
      lcdScrollPreviousMillis = millis();
    }
    dataSendPreviousMillis = millis();
  }

  if (shouldDisplayLCD && millis() - lcdScrollPreviousMillis >= lcdScrollInterval) {
    lcdScrollPreviousMillis = millis(); // ✅ Timer dipindah ke atas
    lcd.clear();

    switch (lcdDisplayState) {
      case 0:
        lcd.print("Suhu:");
        lcd.setCursor(6, 0);
        lcd.print(lastTemp, 1);
        lcd.print((char)223);
        lcd.print("C");
        break;

      case 1:
        lcd.print("pH:");
        lcd.setCursor(4, 0);
        lcd.print(lastPH, 2);
        break;

      case 2:
        lcd.print("Kekeruhan:");
        lcd.setCursor(0, 1);
        lcd.print(lastNTU, 0);
        lcd.print(" NTU");
        break;

      default:
        shouldDisplayLCD = false;            
        lcd.print("Monitoring...");         
        break;
    }

    lcdDisplayState++; 
  
}


