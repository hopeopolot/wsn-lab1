#include <Wire.h>
#include <Adafruit_SHT31.h>

// SHT31
#define SDA_PIN 21
#define SCL_PIN 22

// LDR
#define LDR_PIN 34

// PIR
#define PIR_PIN 27

// LED
#define LED_PIN 25

// Buzzer
#define BUZZER_PIN 26

// SHT31 I2C address
#define SHT31_ADDRESS 0x44

// High temperature threshold
#define HIGH_TEMP 30.0


// Create SHT31 object
Adafruit_SHT31 sht31 = Adafruit_SHT31();


void setup() {

  // Start Serial Monitor
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 MULTI-SENSOR NODE");
  Serial.println("================================");

  // -------------------------------------------------
  // Configure pins
  // -------------------------------------------------

  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Make sure outputs start OFF
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // -------------------------------------------------
  // Start I2C
  // -------------------------------------------------

  Wire.begin(SDA_PIN, SCL_PIN);

  // -------------------------------------------------
  // Start SHT31
  // -------------------------------------------------

  if (!sht31.begin(SHT31_ADDRESS)) {

    Serial.println("ERROR: SHT31 NOT FOUND!");

  } 
  else {

    Serial.println("SHT31 detected successfully!");

  }

  Serial.println("--------------------------------");
  Serial.println("System ready.");
  Serial.println("--------------------------------");
}


void loop() {

  // =================================================
  // 1. READ TEMPERATURE AND HUMIDITY
  // =================================================

  float temperature = sht31.readTemperature();
  float humidity = sht31.readHumidity();


  // =================================================
  // 2. READ LDR
  // =================================================

  int ldrValue = analogRead(LDR_PIN);

  // Convert ADC value to voltage
  float lightVoltage = (ldrValue / 4095.0) * 3.3;


  // =================================================
  // 3. READ PIR
  // =================================================

  int motion = digitalRead(PIR_PIN);


  // =================================================
  // 4. HIGH TEMPERATURE DECISION
  // =================================================

  if (!isnan(temperature)) {

    if (temperature >= HIGH_TEMP) {

      digitalWrite(LED_PIN, HIGH);

    } 
    else {

      digitalWrite(LED_PIN, LOW);

    }

  }


  // =================================================
  // 5. MOTION DECISION
  // =================================================

  if (motion == HIGH) {

    digitalWrite(BUZZER_PIN, HIGH);

  } 
  else {

    digitalWrite(BUZZER_PIN, LOW);

  }


  // =================================================
  // 6. DISPLAY RESULTS
  // =================================================

  Serial.println();

  Serial.println("----------- SENSOR DATA -----------");


  // Temperature
  if (isnan(temperature)) {

    Serial.println("Temperature: ERROR");

  } 
  else {

    Serial.print("Temperature: ");
    Serial.print(temperature, 2);
    Serial.println(" °C");

  }


  // Humidity
  if (isnan(humidity)) {

    Serial.println("Humidity: ERROR");

  } 
  else {

    Serial.print("Humidity: ");
    Serial.print(humidity, 2);
    Serial.println(" %");

  }


  // LDR
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  Serial.print("Light Voltage: ");
  Serial.print(lightVoltage, 2);
  Serial.println(" V");


  // PIR
  if (motion == HIGH) {

    Serial.println("Motion: DETECTED");

  } 
  else {

    Serial.println("Motion: NOT DETECTED");

  }


  // LED status
  if (!isnan(temperature) && temperature >= HIGH_TEMP) {

    Serial.println("Temperature Status: HIGH");
    Serial.println("LED: ON");

  } 
  else {

    Serial.println("Temperature Status: NORMAL");
    Serial.println("LED: OFF");

  }


  // Buzzer status
  if (motion == HIGH) {

    Serial.println("Buzzer: ON");

  } 
  else {

    Serial.println("Buzzer: OFF");

  }


  Serial.println("-----------------------------------");


  // Sample approximately every 1 second
  delay(1000);
}