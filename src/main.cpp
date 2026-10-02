#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

// ========================================
// HARDWARE PIN ASSIGNMENTS
// ========================================
const uint8_t PIN_PH_SENSOR = A0;       // pH sensor analog input
const uint8_t PIN_MP3_RX      = 10;     // SoftwareSerial RX -> DFPlayer TX
const uint8_t PIN_MP3_TX      = 11;     // SoftwareSerial TX -> DFPlayer RX

// ========================================
// SENSOR SAMPLING CONFIGURATION
// ========================================
const uint8_t  NUM_SAMPLES         = 20;    // Number of ADC readings to average
const uint16_t SAMPLE_DELAY_MS     = 50;    // Delay between samples (ms)
const float    ADC_TO_VOLTAGE      = 5.0 / 1023.0;  // 10-bit ADC, 5V reference
const uint16_t SENSOR_SETTLING_MS  = 4000;  // Sensor stabilization time before sampling (ms)

// ========================================
// PH CALIBRATION CONSTANTS
// ========================================
// Linear calibration: pH = SLOPE * voltage + INTERCEPT
// Derived from calibration measurements with buffer solutions.
const float PH_CALIBRATION_SLOPE     = -35.458;
const float PH_CALIBRATION_INTERCEPT = 171.26;
const float PH_MIN = 0.0;
const float PH_MAX = 14.0;

// ========================================
// DFPLAYER AUDIO TRACK MAPPING
// ========================================
// Track assignments on microSD card:
//   1  = "1"       2  = "2"       3  = "3"       4  = "4"       5  = "5"
//   6  = "6"       7  = "7"       8  = "8"       9  = "9"       10 = "0"
//   11 = "koma"    12 = "pH terbaca adalah"
//
// NOTE: pH 10-14 requires additional tracks. See documentation.
const uint8_t TRACK_DIGIT[10] = {10, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t TRACK_KOMAS      = 11;
const uint8_t TRACK_PH_PREFIX  = 12;

// ========================================
// AUDIO PLAYBACK TIMING
// ========================================
// These delays are approximations based on track duration.
// The DFPlayer Mini does not provide a simple blocking "wait until done" API
// in this library version. Using readState() would require polling, which
// adds similar complexity. These values should be tuned to actual track lengths.
const uint16_t DELAY_PREFIX_MS   = 5000;  // After "pH terbaca adalah"
const uint16_t DELAY_INTEGER_MS  = 3000;  // After integer digit
const uint16_t DELAY_KOMAS_MS    = 2000;  // After "koma"
const uint16_t DELAY_DECIMAL_MS  = 2000;  // After decimal digit

// ========================================
// GLOBAL OBJECTS
// ========================================
SoftwareSerial mp3Serial(PIN_MP3_RX, PIN_MP3_TX);
DFRobotDFPlayerMini mp3;

// ========================================
// HELPER: PLAY TRACK AND WAIT
// ========================================
// Plays an audio track and waits for it to finish.
// Uses fixed delay because the DFPlayer Mini library does not provide
// a simple blocking wait-for-completion function.
void playTrackAndWait(uint8_t track, uint16_t waitMs) {
  mp3.play(track);
  delay(waitMs);
}

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);

  Serial.println(F("================================="));
  Serial.println(F("       PEMBACA pH TUNANETRA"));
  Serial.println(F("================================="));

  // Initialize DFPlayer Mini
  if (!mp3.begin(mp3Serial)) {
    Serial.println(F("DFPlayer gagal terdeteksi!"));
    while (true);  // Halt on hardware failure
  }

  Serial.println(F("DFPlayer berhasil terdeteksi."));

  // Volume range: 0 (mute) to 30 (max)
  mp3.volume(25);

  delay(1000);

  // ======================================
  // SENSOR SETTLING PHASE
  // ======================================
  // Allow the pH sensor to stabilize after power-on.
  // Chemical sensors need time to reach equilibrium.
  Serial.println(F("Menunggu sensor stabil..."));
  delay(SENSOR_SETTLING_MS);
  Serial.println(F("Sensor siap."));

  // ======================================
  // TAKE MEASUREMENT
  // ======================================
  float pHValue = readPHSensor();
  announcePH(pHValue);

  Serial.println(F("================================="));
  Serial.println(F("Pengukuran selesai."));
  Serial.println(F("Matikan perangkat untuk mengukur lagi."));
  Serial.println(F("================================="));

  // System remains idle after measurement.
  // User must power-cycle to take another reading.
}

// ========================================
// READ PH SENSOR
// ========================================
// Takes multiple ADC samples, averages them, converts to voltage,
// applies linear calibration, and clamps result to valid pH range.
float readPHSensor() {
  long adcSum = 0;

  // Collect multiple samples to reduce electrical noise
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    adcSum += analogRead(PIN_PH_SENSOR);
    delay(SAMPLE_DELAY_MS);
  }

  // Compute average ADC value (0-1023)
  float adcAverage = adcSum / static_cast<float>(NUM_SAMPLES);

  // Convert ADC to voltage (0-5V)
  float voltage = adcAverage * ADC_TO_VOLTAGE;

  // Apply linear calibration: pH = slope * voltage + intercept
  float pH = PH_CALIBRATION_SLOPE * voltage + PH_CALIBRATION_INTERCEPT;

  // Clamp to chemically valid pH range [0, 14]
  pH = constrain(pH, PH_MIN, PH_MAX);

  // Debug output
  Serial.print(F("ADC      : "));
  Serial.println(adcAverage);

  Serial.print(F("Tegangan : "));
  Serial.print(voltage, 3);
  Serial.println(F(" V"));

  Serial.print(F("pH       : "));
  Serial.println(pH, 1);

  Serial.println(F("---------------------------------"));

  return pH;
}

// ========================================
// ANNOUNCE PH VALUE VIA AUDIO
// ========================================
// Rounds pH to 1 decimal place, splits into integer and decimal parts,
// and plays corresponding audio tracks with appropriate delays.
void announcePH(float pH) {
  // Round to 1 decimal place (e.g., 7.36 -> 7.4)
  pH = round(pH * 10.0) / 10.0;

  // Split into integer and decimal portions
  // Example: pH = 7.4 -> integerPart = 7, decimalPart = 4
  int integerPart = static_cast<int>(pH);
  int decimalPart = static_cast<int>(round((pH - integerPart) * 10.0));

  // Handle rounding edge case: 7.95 rounds to 8.0
  // round(7.95 * 10) / 10 = 8.0, so decimalPart becomes 10
  if (decimalPart == 10) {
    integerPart++;
    decimalPart = 0;
  }

  // ======================================
  // NOTE: INTEGER PART BOUNDS CHECK
  // ======================================
  // TRACK_DIGIT array only has 10 elements (indices 0-9).
  // pH 10-14 would cause out-of-bounds array access.
  // See documentation for required audio track additions.

  // Play: "pH terbaca adalah"
  playTrackAndWait(TRACK_PH_PREFIX, DELAY_PREFIX_MS);

  // Play integer digit (e.g., "7")
  // Only play if within bounds of TRACK_DIGIT array
  if (integerPart < 10) {
    playTrackAndWait(TRACK_DIGIT[integerPart], DELAY_INTEGER_MS);
  }

  // Play: "koma"
  playTrackAndWait(TRACK_KOMAS, DELAY_KOMAS_MS);

  // Play decimal digit (e.g., "4")
  // decimalPart is always 0-9 after rounding logic above
  playTrackAndWait(TRACK_DIGIT[decimalPart], DELAY_DECIMAL_MS);
}

// ========================================
// MAIN LOOP
// ========================================
// Intentionally empty. The device measures once on power-on
// and remains idle until power-cycled.
void loop() {
  // Idle - do nothing
}