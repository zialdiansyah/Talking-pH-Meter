#include <Arduino.h>

#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <math.h>

// ========================================
// PIN
// ========================================

#define PIN_SENSOR_PH A0

// Arduino D10 = RX
// Arduino D11 = TX
SoftwareSerial mySerial(10, 11);

DFRobotDFPlayerMini mp3;


// ========================================
// MAPPING TRACK ANGKA
// ========================================

// Track 1  = "1"
// Track 2  = "2"
// Track 3  = "3"
// Track 4  = "4"
// Track 5  = "5"
// Track 6  = "6"
// Track 7  = "7"
// Track 8  = "8"
// Track 9  = "9"
// Track 10 = "0"
// Track 11 = "koma"
// Track 12 = "pH terbaca adalah"

int trackAngka[10] = {
  10,  // 0
  1,   // 1
  2,   // 2
  3,   // 3
  4,   // 4
  5,   // 5
  6,   // 6
  7,   // 7
  8,   // 8
  9    // 9
};


// ========================================
// SETUP
// ========================================

void setup() {

  Serial.begin(9600);
  mySerial.begin(9600);

  Serial.println("=================================");
  Serial.println("       PEMBACA pH TUNANETRA");
  Serial.println("=================================");

  // Hubungkan DFPlayer
  if (!mp3.begin(mySerial)) {

    Serial.println("DFPlayer gagal terdeteksi!");

    while (true);
  }

  Serial.println("DFPlayer berhasil terdeteksi.");

  // Volume 0 - 30
  mp3.volume(25);

  delay(1000);

  Serial.println("Sistem siap.");
}


// ========================================
// MEMBACA SENSOR pH
// ========================================

float bacaSensorPH() {

  long totalADC = 0;

  // Mengambil 20 sampel
  for (int i = 0; i < 20; i++) {

    totalADC += analogRead(PIN_SENSOR_PH);

    delay(20);
  }

  // Rata-rata ADC
  float adcAverage = totalADC / 20.0;

  // Konversi ADC → tegangan
  float voltage = adcAverage * (5.0 / 1023.0);

  // ======================================
  // PERSAMAAN KALIBRASI
  // ======================================

  float pH = -35.458 * voltage + 171.26;


  // ======================================
  // BATAS pH
  // ======================================

  if (pH < 0) {
    pH = 0;
  }

  if (pH > 14) {
    pH = 14;
  }


  // ======================================
  // SERIAL MONITOR
  // ======================================

  Serial.print("ADC      : ");
  Serial.println(adcAverage);

  Serial.print("Tegangan : ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  Serial.print("pH       : ");
  Serial.println(pH, 1);

  Serial.println("---------------------------------");


  return pH;
}


// ========================================
// MENGUCAPKAN HASIL pH
// ========================================

void ucapkanPH(float pH) {

  // Bulatkan menjadi 1 angka desimal
  pH = round(pH * 10) / 10.0;


  // ======================================
  // PISAHKAN ANGKA
  // ======================================

  int satuan = (int)pH;

  int desimal = (int)round(
    (pH - satuan) * 10
  );


  // ======================================
  // MENANGANI PEMBULATAN
  // ======================================

  if (desimal == 10) {

    satuan++;
    desimal = 0;
  }


  // ======================================
  // "pH TERBACA ADALAH"
  // ======================================

  mp3.play(12);
  delay(5000);


  // ======================================
  // ANGKA SEBELUM KOMA
  // ======================================

  mp3.play(trackAngka[satuan]);
  delay(3000);


  // ======================================
  // "KOMA"
  // ======================================

  mp3.play(11);
  delay(2000);


  // ======================================
  // ANGKA SETELAH KOMA
  // ======================================

  mp3.play(trackAngka[desimal]);
  delay(2000);
}


// ========================================
// LOOP UTAMA
// ========================================

void loop() {

  // Baca sensor
  float pHterbaca = bacaSensorPH();

  // Ucapkan hasil pH
  ucapkanPH(pHterbaca);

  // Tunggu 10 detik sebelum membaca lagi
  delay(10000);
}
