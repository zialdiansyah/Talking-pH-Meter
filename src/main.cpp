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
const uint8_t  NUM_SAMPLES       = 20;  // Number of ADC readings to average
const uint16_t SAMPLE_DELAY_MS   = 20;  // Delay between samples (ms)
const float    ADC_TO_VOLTAGE    = 5.0 / 1023.0;  // 10-bit ADC, 5V reference

// ========================================
// PH CALIBRATION CONSTANTS
// ========================================
// Linear calibration: pH = SLOPE * voltage + INTERCEPT
// Derived from calibration measurements with buffer solutions.
const float PH_CALIBRATION_SLOPE     = -6.4567;
const float PH_CALIBRATION_INTERCEPT = 32.223;
const float PH_MIN = 0.0;
const float PH_MAX = 14.0;

// ========================================
// DFPlayer AUDIO TRACK MAPPING
// ========================================
// Track assignments on microSD card:
//   001 = nol              002 = satu            003 = dua
//   004 = tiga             005 = empat           006 = lima
//   007 = enam             008 = tujuh           009 = delapan
//   010 = sembilan         011 = sepuluh         012 = sebelas
//   013 = dua belas        014 = tiga belas      015 = empat belas
//   016 = koma             017 = "pH terbaca adalah"
const uint8_t TRACK_NOL            = 33;
const uint8_t TRACK_SATU           = 1;
const uint8_t TRACK_DUA            = 3;
const uint8_t TRACK_TIGA           = 5;
const uint8_t TRACK_EMPAT          = 7;
const uint8_t TRACK_LIMA           = 9;
const uint8_t TRACK_ENAM           = 11;
const uint8_t TRACK_TUJUH          = 13;
const uint8_t TRACK_DELAPAN        = 15;
const uint8_t TRACK_SEMBILAN       = 17;
const uint8_t TRACK_SEPULUH        = 19;
const uint8_t TRACK_SEBELAS        = 21;
const uint8_t TRACK_DUA_BELAS      = 23;
const uint8_t TRACK_TIGA_BELAS     = 25;
const uint8_t TRACK_EMPAT_BELAS    = 27;
const uint8_t TRACK_KOMA           = 29;
const uint8_t TRACK_PH_PREFIX      = 31;

// ========================================
// AUDIO PLAYBACK CONFIGURATION
// ========================================
// Timeout for waiting for DFPlayer playback-finished event (ms)
const uint32_t PLAYBACK_TIMEOUT_MS = 30000;

// ========================================
// MAIN LOOP TIMING
// ========================================
const uint32_t LOOP_INTERVAL_MS = 10000;  // Time between pH readings

// ========================================
// GLOBAL OBJECTS
// ========================================
SoftwareSerial mp3Serial(PIN_MP3_RX, PIN_MP3_TX);
DFRobotDFPlayerMini mp3;

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
  mp3.volume(26);

  delay(1000);
  Serial.println(F("Sistem siap."));
}

// ========================================
// READ PH SENSOR
// ========================================
// Takes multiple ADC samples, averages them, converts to voltage,
// applies linear calibration, and clamps result to valid pH range.
float readPHSensor() {
  long adcSum = 0;

  // Collect multiple samples to reduce noise
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    adcSum += analogRead(PIN_PH_SENSOR);
    delay(SAMPLE_DELAY_MS);
  }

  // Compute average ADC value
  float adcAverage = adcSum / static_cast<float>(NUM_SAMPLES);

  // Convert ADC (0-1023) to voltage (0-5V)
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
// PLAY AUDIO TRACK AND WAIT FOR COMPLETION
// ========================================
// Plays the specified track and waits for DFPlayerPlayFinished event.
// Returns true on successful completion, false on timeout or error.
bool playAndWait(uint8_t track) {
  mp3.play(track);

  uint32_t startTime = millis();
  while (millis() - startTime < PLAYBACK_TIMEOUT_MS) {
    if (mp3.available()) {
      uint8_t type = mp3.readType();
      int value = mp3.read();

      if (type == DFPlayerPlayFinished) {
        return true;  // Track finished playing
      }
      if (type == DFPlayerError) {
        Serial.print(F("DFPlayer error: "));
        Serial.println(value);
        return false;  // Playback error
      }
    }
    // Small yield to avoid tight loop
    delay(10);
  }

  Serial.println(F("Playback timeout"));
  return false;  // Timeout
}

// ========================================
// ANNOUNCE PH VALUE VIA AUDIO
// ========================================
// Rounds pH to 1 decimal place, splits into integer and decimal parts,
// and plays corresponding audio tracks using event-based synchronization.
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

  // Play: "pH terbaca adalah"
  playAndWait(TRACK_PH_PREFIX);

  // Play integer part (0-14) with natural Indonesian pronunciation
  uint8_t integerTrack;
  switch (integerPart) {
    case 0:  integerTrack = TRACK_NOL;           break;
    case 1:  integerTrack = TRACK_SATU;          break;
    case 2:  integerTrack = TRACK_DUA;           break;
    case 3:  integerTrack = TRACK_TIGA;          break;
    case 4:  integerTrack = TRACK_EMPAT;         break;
    case 5:  integerTrack = TRACK_LIMA;          break;
    case 6:  integerTrack = TRACK_ENAM;          break;
    case 7:  integerTrack = TRACK_TUJUH;         break;
    case 8:  integerTrack = TRACK_DELAPAN;       break;
    case 9:  integerTrack = TRACK_SEMBILAN;      break;
    case 10: integerTrack = TRACK_SEPULUH;       break;
    case 11: integerTrack = TRACK_SEBELAS;       break;
    case 12: integerTrack = TRACK_DUA_BELAS;     break;
    case 13: integerTrack = TRACK_TIGA_BELAS;    break;
    case 14: integerTrack = TRACK_EMPAT_BELAS;   break;
    default: integerTrack = TRACK_NOL;           break;
  }
  playAndWait(integerTrack);

  // Always play decimal part: "koma" + digit (including 0)
  playAndWait(TRACK_KOMA);

  uint8_t decimalTrack;
  switch (decimalPart) {
    case 0: decimalTrack = TRACK_NOL;      break;
    case 1: decimalTrack = TRACK_SATU;     break;
    case 2: decimalTrack = TRACK_DUA;      break;
    case 3: decimalTrack = TRACK_TIGA;     break;
    case 4: decimalTrack = TRACK_EMPAT;    break;
    case 5: decimalTrack = TRACK_LIMA;     break;
    case 6: decimalTrack = TRACK_ENAM;     break;
    case 7: decimalTrack = TRACK_TUJUH;    break;
    case 8: decimalTrack = TRACK_DELAPAN;  break;
    case 9: decimalTrack = TRACK_SEMBILAN; break;
    default: decimalTrack = TRACK_NOL;     break;
  }
  playAndWait(decimalTrack);
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  float pHValue = readPHSensor();
  announcePH(pHValue);
  delay(LOOP_INTERVAL_MS);
}