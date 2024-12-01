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
#define WATER_LEVEL_THRESHOLD 50
#define SOIL_THRESHOLD 30 // Percent
#define LOW_LIGHT_THRESHOLD 1000
#define HIGH_TEMP_THRESHOLD 35
#define LOW_TEMP_THRESHOLD 10
#define LOW_HUMIDITY_THRESHOLD 40
#define HIGH_HUMIDITY_THRESHOLD 70

// Blynk
char auth[] = "NbERJsrZtO8F_L2Ajs_dRpZF-pkbCUiC";
char ssid[] = "Kaesenbangbang";
char pass[] = "1212312121";

// Google Sheets
bool uploadEnable = false;
unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 300000;

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
  digitalWrite(PUMP_RELAY_PIN, HIGH); // Pump is OFF
}

void loop() {

  Blynk.run();

  unsigned long currentTime = millis();

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "enable") {
      uploadEnable = true;
      Serial.println("Upload Enabled.");
    } else if (command == "C") {
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
  Serial.println(waterValue);
  int waterValueNew = map(waterValue,0,2100,0,100);
  Serial.print("Water Level: ");
  Serial.println(waterValueNew);
  Blynk.virtualWrite(V3, waterValueNew);

    // Read LDR Sensor
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("Light Intensity: ");
  Serial.println(ldrValue);
  Blynk.virtualWrite(V4, ldrValue);

  // Light Intensity Logic
  if (ldrValue < LOW_LIGHT_THRESHOLD) {
    Serial.println("Warning: Insufficient light for plant growth. Consider relocating the plant.");
  }

  // Soil Moisture and Water Level Logic
  if (soilMoisturePercent < SOIL_THRESHOLD && waterValueNew > WATER_LEVEL_THRESHOLD) {
    int wateringDuration = 5000; // Default watering duration

    // Adjust watering based on humidity
    if (humidity < LOW_HUMIDITY_THRESHOLD) {
      wateringDuration = 8000; 
    } else if (humidity > HIGH_HUMIDITY_THRESHOLD) {
      wateringDuration = 3000; 
    } 

    Serial.println("Soil is dry and water is available. Watering plant...");
    digitalWrite(PUMP_RELAY_PIN, LOW); // Pump is ON
    delay(wateringDuration);
    digitalWrite(PUMP_RELAY_PIN, HIGH); // Pump is OFF
    Serial.println("Watering complete.");
  } else if (soilMoisturePercent >= SOIL_THRESHOLD) {
    Serial.println("Soil moisture is sufficient. No watering needed.");
  } else {
    Serial.println("Water level too low. Cannot water the plant.");
  }

  // Google Sheets
  if (uploadEnable) {
    if (currentTime - lastUploadTime >= uploadInterval) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("YOUR_WEBHOOK_URL");
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
    lastUploadTime = currentTime;
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
    digitalWrite(PUMP_RELAY_PIN, LOW); // Pump is ON
  } else {
    Serial.println("Manual watering deactivated.");
    digitalWrite(PUMP_RELAY_PIN, HIGH); // Pump is OFF
  }
}