#include <DHT.h>

// ---------------- Pins ----------------
#define DHTPIN   15
#define DHTTYPE  DHT22
#define LDR_PIN  34      // ADC1, input-only
#define PIR_PIN  27
#define LED_PIN  2

// ---------------- Calibration ----------------
// T_cal = CAL_GAIN * T_raw + CAL_OFFSET
// Placeholders: replace with values fitted from reference-thermometer data.
const float CAL_GAIN   = 1.0;
const float CAL_OFFSET = 0.0;

// ---------------- Thresholds ----------------
const float TEMP_HIGH_C  = 30.0;
const float TEMP_LOW_C   = 18.0;
const float HUM_HIGH_PCT = 70.0;
const float HUM_LOW_PCT  = 30.0;

// ---------------- Timing / filter ----------------
const unsigned long SAMPLE_MS = 1000;   // 1 Hz
const int N = 5;                        // moving-average window

// ---------------- State ----------------
DHT dht(DHTPIN, DHTTYPE);
float buf[N];
int bufIdx = 0, bufCount = 0;
float prevFilt = NAN;
unsigned long lastSample = 0;
int prevMotion = 0;

float movingAverage(float x) {
  buf[bufIdx] = x;
  bufIdx = (bufIdx + 1) % N;
  if (bufCount < N) bufCount++;
  float s = 0;
  for (int i = 0; i < bufCount; i++) s += buf[i];
  return s / bufCount;
}

// Builds a status string such as "NORMAL", "HIGH_TEMP", "LOW_TEMP+HIGH_HUM"
// Returns true if any alarm is active.
bool buildStatus(float tCal, float h, char *out, size_t len) {
  out[0] = '\0';
  bool alarm = false;

  if (tCal > TEMP_HIGH_C)      { strncat(out, "HIGH_TEMP", len - strlen(out) - 1); alarm = true; }
  else if (tCal < TEMP_LOW_C)  { strncat(out, "LOW_TEMP",  len - strlen(out) - 1); alarm = true; }

  const char *humTag = nullptr;
  if (h > HUM_HIGH_PCT)        humTag = "HIGH_HUM";
  else if (h < HUM_LOW_PCT)    humTag = "LOW_HUM";

  if (humTag) {
    if (alarm) strncat(out, "+", len - strlen(out) - 1);
    strncat(out, humTag, len - strlen(out) - 1);
    alarm = true;
  }

  if (!alarm) {
    strncpy(out, "NORMAL", len - 1);
    out[len - 1] = '\0';
  }
  return alarm;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  analogReadResolution(12);
  dht.begin();

  // '#' lines are comments: skip them in pandas with comment='#'
  Serial.println("# Sensor node started");
  Serial.println("timestamp,temp_raw,temp_cal,temp_filt,d_temp,humidity,light,motion,motion_event,valid,status");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;

  // ---- Acquire ----
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  float light = analogRead(LDR_PIN) / 4095.0;   // normalised 0..1
  int motion = digitalRead(PIR_PIN);

  // ---- Validate (plausibility limits) ----
  bool valid = !(isnan(t) || isnan(h)) &&
               t > -40 && t < 80 && h >= 0 && h <= 100;

  float tCal = NAN, tFilt = NAN, dT = NAN;
  char status[32] = "INVALID";
  bool alarm = false;

  // ---- Edge processing ----
  if (valid) {
    tCal  = CAL_GAIN * t + CAL_OFFSET;
    tFilt = movingAverage(tCal);
    if (!isnan(prevFilt)) dT = tFilt - prevFilt;
    prevFilt = tFilt;
    alarm = buildStatus(tCal, h, status, sizeof(status));
  }

  // ---- Event detection: PIR rising edge ----
  int motionEvent = (motion == 1 && prevMotion == 0) ? 1 : 0;
  prevMotion = motion;

  // ---- Local indicator: LED on for any alarm or motion ----
  digitalWrite(LED_PIN, alarm || motion);

  // ---- Machine-readable CSV line ----
  Serial.printf("%lu,%.2f,%.2f,%.2f,%.3f,%.1f,%.3f,%d,%d,%d,%s\n",
                now / 1000, t, tCal, tFilt, dT, h, light,
                motion, motionEvent, valid ? 1 : 0, status);
}