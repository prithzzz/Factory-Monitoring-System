#include <DHT.h>

// -------- PIN CONFIG --------
#define TEMP1_PIN A0
#define GAS_PIN   A2
#define DHT_PIN   4
#define IR_PIN    3    

#define GREEN  6
#define YELLOW 7
#define RED    2

// -------- DHT SETUP --------
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// -------- AVERAGING CONFIG --------
#define NUM_SAMPLES 10

// -------- VARIABLES --------
float temp1, humidity, gasValue;
bool  irDetected;

float baseTemp1, baseHumidity, baseGas;
float warnTemp1,  dangerTemp1;
float warnHumidity, dangerHumidity;
float warnGas,    dangerGas;

// -------- HELPER: Averaged Analog Read --------
float readAvgAnalog(int pin) {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return (float)sum / NUM_SAMPLES;
}

// -------- HELPER: Averaged DHT Humidity --------
float readAvgHumidity() {
  float sum = 0;
  int valid = 0;
  for (int i = 0; i < 5; i++) {
    float h = dht.readHumidity();
    if (!isnan(h)) {
      sum += h;
      valid++;
    }
    delay(300);
  }
  if (valid == 0) return 50.0;
  return sum / valid;
}

void setup() {
  Serial.begin(9600);

  pinMode(GREEN,  OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED,    OUTPUT);
  pinMode(IR_PIN, INPUT);   // LM393 DO is an output from sensor, input to Arduino

  digitalWrite(GREEN,  LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED,    LOW);

  dht.begin();

  Serial.println("Warming up sensors... please wait 5 seconds.");
  delay(5000);

  Serial.println("Calibrating...");

  baseTemp1    = readAvgAnalog(TEMP1_PIN) * 0.488;
  baseGas      = readAvgAnalog(GAS_PIN);
  baseHumidity = readAvgHumidity();

  if (baseTemp1    < 1.0)  baseTemp1    = 25.0;
  if (baseGas      < 1.0)  baseGas      = 100.0;
  if (isnan(baseHumidity)) baseHumidity = 50.0;

  warnTemp1    = baseTemp1    * 1.4;   dangerTemp1    = baseTemp1    * 1.7;
  warnHumidity = baseHumidity * 1.2;   dangerHumidity = baseHumidity * 1.4;
  warnGas      = baseGas      * 1.2;   dangerGas      = baseGas      * 1.4;

  Serial.println("=== Calibration Done ===");
  Serial.print("Temp1  base:"); Serial.print(baseTemp1);
  Serial.print("  warn:"); Serial.print(warnTemp1);
  Serial.print("  danger:"); Serial.println(dangerTemp1);

  Serial.print("Hum    base:"); Serial.print(baseHumidity);
  Serial.print("  warn:"); Serial.print(warnHumidity);
  Serial.print("  danger:"); Serial.println(dangerHumidity);

  Serial.print("Gas    base:"); Serial.print(baseGas);
  Serial.print("  warn:"); Serial.print(warnGas);
  Serial.print("  danger:"); Serial.println(dangerGas);

  Serial.println("IR sensor: active (LOW = detected)");
  Serial.println("========================");
}

void loop() {

  // -------- READ SENSORS --------
  temp1    = readAvgAnalog(TEMP1_PIN) * 0.488;
  gasValue = readAvgAnalog(GAS_PIN);
  humidity = dht.readHumidity();

  if (isnan(humidity)) humidity = baseHumidity;

  // LM393 DO is active LOW — pulls LOW when IR beam is interrupted/object detected
  irDetected = (digitalRead(IR_PIN) == LOW);

  // -------- DETERMINE STATUS --------
  bool danger  = false;
  bool warning = false;

  if (temp1    > dangerTemp1    ||
      humidity > dangerHumidity ||
      gasValue > dangerGas      ||
      irDetected) {              // IR detection = immediate danger (intruder/obstruction)
    danger = true;
  } else if (temp1    > warnTemp1    ||
             humidity > warnHumidity ||
             gasValue > warnGas) {
    warning = true;
  }

  // -------- LED CONTROL --------
  digitalWrite(GREEN,  (!danger && !warning) ? HIGH : LOW);
  digitalWrite(YELLOW, (!danger &&  warning) ? HIGH : LOW);
  digitalWrite(RED,    ( danger            ) ? HIGH : LOW);

  // -------- SERIAL OUTPUT --------
  Serial.print("T1:");    Serial.print(temp1,    1);
  Serial.print("  Hum:"); Serial.print(humidity, 1);
  Serial.print("  Gas:"); Serial.print(gasValue, 0);
  Serial.print("  IR:");  Serial.print(irDetected ? "DETECTED" : "clear");

  if      (danger)  Serial.println("  | STATUS: DANGER");
  else if (warning) Serial.println("  | STATUS: WARNING");
  else              Serial.println("  | STATUS: NORMAL");

  delay(1000);
}