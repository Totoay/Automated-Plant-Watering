#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Blynk
#define BLYNK_TEMPLATE_ID "TMPL6VZj-PXBq"
#define BLYNK_TEMPLATE_NAME "Plant Watering System"
#define BLYNK_AUTH_TOKEN "NbERJsrZtO8F_L2Ajs_dRpZF-pkbCUiC"

#include <BlynkSimpleEsp32.h>

// Pin Definitions
#define DHTPIN 33              // DHT11 pin
#define DHTTYPE DHT11          // DHT11 sensor type
#define SOIL_PIN 35            // Soil moisture analog pin
#define WATER_PIN 34           // Water sensor analog pin
#define LDR_PIN 32             // LDR module analog pin
#define PUMP_RELAY_PIN 25      // Water pump relay pin

// Threshold Values
#define SOIL_THRESHOLD 30      // Soil moisture percentage threshold
#define LOW_LIGHT_THRESHOLD 1000
#define HIGH_TEMP_THRESHOLD 35
#define LOW_TEMP_THRESHOLD 10
#define LOW_HUMIDITY_THRESHOLD 30
#define HIGH_HUMIDITY_THRESHOLD 80

char auth[] = "NbERJsrZtO8F_L2Ajs_dRpZF-pkbCUiC";
char ssid[] = "Kaesenbangbang";
char pass[] = "1212312121";

bool uploadEnable = false;

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // analogSetAttenuation(ADC_11db);

  // Blynk and WiFi
  WiFi.begin(ssid,pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting to WiFi....");
  }
  Serial.println("Connected to WiFi");
  Blynk.begin(auth,ssid,pass);
  
  // Initialize DHT Sensor
  dht.begin();

  // Initialize Pump Relay  
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Ensure pump is off initially
}

void loop() {

  Blynk.run();

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "enable") {
      uploadEnable = true;
      Serial.println("Upload Enabled.");
    } else if (command == "disable") {
      uploadEnable = false;
      Serial.println("Upload Disabled.");
    }
  }

  // Read DHT11 Sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    Blynk.virtualWrite(V0, temperature);

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Blynk.virtualWrite(V1, humidity);

    // Temperature and Humidity Logic
    if (temperature > HIGH_TEMP_THRESHOLD) {
      Serial.println("Warning: High temperature detected. Ensure adequate watering and shading.");
    } else if (temperature < LOW_TEMP_THRESHOLD) {
      Serial.println("Warning: Low temperature detected. Protect the plant from cold.");
    }

    if (humidity < LOW_HUMIDITY_THRESHOLD) {
      Serial.println("Low humidity detected. Increase watering frequency.");
    } else if (humidity > HIGH_HUMIDITY_THRESHOLD) {
      Serial.println("High humidity detected. Reduce watering frequency.");
    }
  }
  // Read Soil Moisture Sensor
  int soilValue = analogRead(SOIL_PIN);
  int soilMoisturePercent = map(soilValue, 0, 4095, 0, 100);
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");
  Blynk.virtualWrite(V2, soilMoisturePercent);

  // Read Water Sensor
  int waterValue = analogRead(WATER_PIN);
  int waterValueNew = map(waterValue,0,4095,0,1023);
  Serial.print("Water Level: ");
  Serial.println(waterValueNew);
  Blynk.virtualWrite(V3, waterValueNew);

  // Soil Moisture and Water Level Logic
  if (soilMoisturePercent < SOIL_THRESHOLD && waterValueNew > 500) {
    Serial.println("Soil is dry and water is available. Watering plant...");
    digitalWrite(PUMP_RELAY_PIN, LOW); // Turn on the pump
    delay(5000); // Watering duration (adjust as needed)
    digitalWrite(PUMP_RELAY_PIN, HIGH);  // Turn off the pump
    Serial.println("Watering complete.");
  } else if (soilMoisturePercent >= SOIL_THRESHOLD) {
    Serial.println("Soil moisture is sufficient. No watering needed.");
  } else {
    Serial.println("Water level too low. Cannot water the plant.");
  }

  // Read LDR Sensor
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("Light Intensity: ");
  Serial.println(ldrValue);
  Blynk.virtualWrite(V4, ldrValue);

  // Light Intensity Logic
  if (ldrValue < LOW_LIGHT_THRESHOLD) {
    Serial.println("Warning: Insufficient light for plant growth. Consider relocating the plant.");
  }

  // Google Sheets
  if (uploadEnable) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("https://script.google.com/macros/s/AKfycbxSydrgDClW2hwk-2BDj3s_0YamJRiR5G2vgHzLahHDexpAhY_DwjfH5Mxn6yxdgidi/exec");
      http.addHeader("Content-Type", "application/json");
      String jsonPayload = String("{\"temperature\":") + temperature +
                     ",\"humidity\":" + humidity +
                     ",\"soil_moisture\":" + soilMoisturePercent +
                     ",\"light_intensity\":" + ldrValue +
                     ",\"water_level\":" + waterValue + "}";
      int httpResponseCode = http.POST(jsonPayload);
      if (httpResponseCode > 0) {
        Serial.println("Data sent successfully");
      } else {
        Serial.print("Error sending data: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
  } else {
    Serial.println("Data upload is disabled.");
  }

  // Delay Between Loops
  Serial.println("-----------------------------");
  delay(5000);
}

BLYNK_WRITE(V5) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    Serial.println("Manual watering activated.");
    digitalWrite(PUMP_RELAY_PIN, LOW); // Turn on the pump
  } else {
    Serial.println("Manual watering deactivated.");
    digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn off the pump
  }
}