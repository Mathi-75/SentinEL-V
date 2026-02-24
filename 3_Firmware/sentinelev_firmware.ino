// ============================================================
//  SentinEL-V v2.0 — Upgraded Edge AI Firmware
//  Board : VSDSquadron ULTRA (THEJAS32 RISC-V)
//  Points: Adaptive Baseline (#2), Kalman Filter (#12), SoH (#1)
// ============================================================

#include <math.h>
#include <EEPROM.h>   // For SoH persistence across reboots

// ─── PINS ────────────────────────────────────────────────────
#define NTC_PIN      A0
#define GAS_PIN      A1
#define BUZZER_PIN   6
#define LED_GREEN    19
#define LED_RED      16
#define BTN_RESET    7   // Manual fault reset button

// ─── NTC CONSTANTS ───────────────────────────────────────────
const float BETA      = 3950.0;
const float R_FIXED   = 10000.0;
const float T0        = 298.15;   // 25°C in Kelvin
const float TEMP_REF  = 10000.0;  // NTC resistance at 25°C

// ─── EEPROM LAYOUT ───────────────────────────────────────────
// Addr 0-1 : uint16_t  stress_count  (thermal stress events)
// Addr 2   : uint8_t   soh_percent   (last known SoH %)
#define EEPROM_STRESS_ADDR   0
#define EEPROM_SOH_ADDR      2

// ─── KALMAN FILTER STATE ─────────────────────────────────────
struct Kalman {
  float x;   // State estimate
  float p;   // Estimate error covariance
  float q;   // Process noise covariance  (tune: lower = smoother)
  float r;   // Measurement noise covariance (tune: higher = smoother)
};

Kalman kf_temp = {25.0, 1.0, 0.01, 2.0};
Kalman kf_gas  = {263.0, 1.0, 0.05, 5.0};

float kalman_update(Kalman &kf, float measurement) {
  // Predict
  float p_pred = kf.p + kf.q;
  // Update
  float k = p_pred / (p_pred + kf.r);          // Kalman gain
  kf.x = kf.x + k * (measurement - kf.x);      // State update
  kf.p = (1.0 - k) * p_pred;                   // Covariance update
  return kf.x;
}

// ─── ADAPTIVE BASELINE ───────────────────────────────────────
#define BASELINE_SAMPLES 60   // 30 seconds at 2Hz = 60 samples
int    gas_samples[BASELINE_SAMPLES];
float  temp_samples[BASELINE_SAMPLES];
int    baseline_idx   = 0;
bool   baseline_ready = false;
float  gas_baseline   = 263.0;
float  temp_baseline  = 25.0;

void update_baseline(float temp, float gas) {
  if (baseline_ready) return;
  temp_samples[baseline_idx] = temp;
  gas_samples[baseline_idx]  = (int)gas;
  baseline_idx++;
  if (baseline_idx >= BASELINE_SAMPLES) {
    // Compute mean
    float tsum = 0, gsum = 0;
    for (int i = 0; i < BASELINE_SAMPLES; i++) {
      tsum += temp_samples[i];
      gsum += gas_samples[i];
    }
    temp_baseline = tsum / BASELINE_SAMPLES;
    gas_baseline  = gsum / BASELINE_SAMPLES;
    baseline_ready = true;
    Serial.print("BASELINE_SET,");
    Serial.print(temp_baseline); Serial.print(",");
    Serial.println(gas_baseline);
  }
}

// ─── TEMPERATURE HISTORY (for dT/dt) ─────────────────────────
#define HIST_LEN 10
float temp_history[HIST_LEN];
int   hist_ptr = 0;

void push_temp(float t) {
  temp_history[hist_ptr % HIST_LEN] = t;
  hist_ptr++;
}

float get_rate_of_rise() {
  if (hist_ptr < HIST_LEN) return 0.0;
  // Slope over last HIST_LEN samples (simple linear regression)
  float sx = 0, sy = 0, sxy = 0, sx2 = 0;
  for (int i = 0; i < HIST_LEN; i++) {
    float xi = i;
    float yi = temp_history[(hist_ptr - HIST_LEN + i) % HIST_LEN];
    sx  += xi; sy  += yi;
    sxy += xi * yi;
    sx2 += xi * xi;
  }
  float n = HIST_LEN;
  return (n * sxy - sx * sy) / (n * sx2 - sx * sx);  // slope = dT/dt
}

// ─── SOH TRACKING ────────────────────────────────────────────
uint16_t stress_count = 0;
uint8_t  soh_percent  = 100;

void load_soh() {
  EEPROM.get(EEPROM_STRESS_ADDR, stress_count);
  if (stress_count > 1000) stress_count = 0;  // Corruption guard
  soh_percent = max(0, 100 - (int)(stress_count / 5));
}

void record_stress_event() {
  stress_count++;
  soh_percent = max(0, 100 - (int)(stress_count / 5));
  EEPROM.put(EEPROM_STRESS_ADDR, stress_count);
  EEPROM.put(EEPROM_SOH_ADDR, soh_percent);
}

// ─── DECISION TREE CLASSIFIER ────────────────────────────────
// Pre-trained tree structure (exported from sklearn, see train_model.py)
// Feature vector: [rate_of_rise, gas_delta]
// Returns: 0=SAFE, 1=PRE-WARNING, 2=WARNING, 3=CRITICAL
int classify(float ror, float gas_delta, float temp) {
  // Node 0: gas_delta <= 80?
  if (gas_delta <= 80.0) {
    // Node 1: ror <= 0.4?
    if (ror <= 0.4) {
      // Node 2: temp <= 38?
      if (temp <= 38.0) return 0;  // SAFE
      else              return 1;  // PRE-WARNING (warm but stable)
    } else {
      // Node 3: ror <= 1.0?
      if (ror <= 1.0) return 1;    // PRE-WARNING
      else {
        if (temp > 40.0) return 3; // CRITICAL — fast rise at high temp
        else             return 2; // WARNING
      }
    }
  } else {
    // Gas elevated
    // Node 4: ror <= 0.6?
    if (ror <= 0.6) return 2;      // WARNING — gas but slow heat
    else            return 3;      // CRITICAL — gas + fast heat
  }
}

// ─── SAFETY LATCH ────────────────────────────────────────────
int  critical_ticks = 0;
bool fault_latched  = false;
#define DEBOUNCE_TICKS 4   // 4 × 500ms = 2s persistence

// ─── RISK SCORE ──────────────────────────────────────────────
int compute_risk(float ror, float gas_delta, float temp) {
  float score = 0;
  score += constrain(ror * 30.0, 0, 45);
  score += constrain(gas_delta * 0.2, 0, 30);
  score += constrain((temp - temp_baseline) * 3.0, 0, 25);
  return (int)constrain(score, 0, 100);
}

// ─── TIME-TO-FAILURE PREDICTION ──────────────────────────────
int predict_ttf(float ror, float current_temp) {
  if (ror <= 0.05) return -1;   // Not rising, no prediction
  float temp_to_critical = 60.0 - current_temp;
  if (temp_to_critical <= 0) return 0;
  // Each sample is 500ms, ror is per sample
  float samples_needed = temp_to_critical / ror;
  return (int)(samples_needed * 0.5);  // Convert to seconds
}

// ─── SETUP ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BTN_RESET,  INPUT_PULLUP);

  load_soh();

  // Init Kalman states from first ADC read
  kf_temp.x = 25.0;
  kf_gas.x  = 263.0;

  // Power-on beep
  digitalWrite(BUZZER_PIN, HIGH); delay(80);
  digitalWrite(BUZZER_PIN, LOW);  delay(80);
  digitalWrite(BUZZER_PIN, HIGH); delay(80);
  digitalWrite(BUZZER_PIN, LOW);

  // Header for dashboard CSV parser
  Serial.println("HEADER,timestamp,temp,ror,gas_raw,class_id,state,risk_score,ttf_sec,soh_pct,baseline_ready");
}

// ─── MAIN LOOP ────────────────────────────────────────────────
void loop() {

  // ── 1. Read sensors ────────────────────────────────────────
  int raw_temp = analogRead(NTC_PIN);
  int raw_gas  = analogRead(GAS_PIN);

  // ── 2. NTC → °C conversion ─────────────────────────────────
  float current_temp = 25.0;
  if (raw_temp > 0 && raw_temp < 2047) {
    float vr    = (float)raw_temp / 2047.0;
    float r_ntc = R_FIXED * ((1.0 / vr) - 1.0);
    float t_k   = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(r_ntc / TEMP_REF));
    current_temp = t_k - 273.15;
  }

  // ── 3. Kalman Filter ───────────────────────────────────────
  float filt_temp = kalman_update(kf_temp, current_temp);
  float filt_gas  = kalman_update(kf_gas,  (float)raw_gas);

  // ── 4. Adaptive Baseline (first 30s) ───────────────────────
  update_baseline(filt_temp, filt_gas);

  // ── 5. Feature Extraction ──────────────────────────────────
  push_temp(filt_temp);
  float ror       = get_rate_of_rise();
  float gas_delta = filt_gas - gas_baseline;

  // ── 6. Decision Tree Classify ──────────────────────────────
  int class_id = 0;
  if (baseline_ready) {
    class_id = classify(ror, gas_delta, filt_temp);
  }
  String state_str = "SAFE";
  if      (class_id == 1) state_str = "PRE-WARNING";
  else if (class_id == 2) state_str = "WARNING";
  else if (class_id == 3) state_str = "CRITICAL";

  // ── 7. Risk Score & TTF ────────────────────────────────────
  int risk_score = compute_risk(ror, gas_delta, filt_temp);
  int ttf_sec    = predict_ttf(ror, filt_temp);

  // ── 8. Safety Latch ────────────────────────────────────────
  if (class_id == 3) {
    critical_ticks++;
    if (critical_ticks >= DEBOUNCE_TICKS) {
      if (!fault_latched) record_stress_event();  // Log to SoH
      fault_latched = true;
    }
  } else {
    critical_ticks = 0;
  }

  // Manual reset button
  if (fault_latched && digitalRead(BTN_RESET) == LOW) {
    fault_latched  = false;
    critical_ticks = 0;
  }

  // ── 9. Actuation ───────────────────────────────────────────
  if (fault_latched) {
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (class_id >= 2) {
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  } else if (class_id == 1) {
    digitalWrite(LED_RED,   HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    // SAFE: heartbeat blink on green
    static bool hb = false;
    hb = !hb;
    digitalWrite(LED_GREEN, hb ? HIGH : LOW);
    digitalWrite(LED_RED,   LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ── 10. UART Telemetry ─────────────────────────────────────
  // Format: DATA,timestamp,temp,ror,gas_raw,class_id,state,risk,ttf,soh,bl_ready
  Serial.print("DATA,");
  Serial.print(millis());       Serial.print(",");
  Serial.print(filt_temp, 2);   Serial.print(",");
  Serial.print(ror, 4);         Serial.print(",");
  Serial.print((int)filt_gas);  Serial.print(",");
  Serial.print(class_id);       Serial.print(",");
  Serial.print(state_str);      Serial.print(",");
  Serial.print(risk_score);     Serial.print(",");
  Serial.print(ttf_sec);        Serial.print(",");
  Serial.print(soh_percent);        Serial.print(",");
  Serial.println(baseline_ready ? 1 : 0);

  delay(500);  // 2 Hz
}
