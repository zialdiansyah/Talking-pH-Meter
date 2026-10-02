# Blind pH Meter

An Arduino-based pH meter designed for visually impaired users that provides audio feedback of pH measurements using text-to-speech via MP3 playback.

## Description

This project measures pH levels using an analog pH sensor and announces the results audibly through a DFPlayer Mini MP3 module. The system is specifically designed to assist visually impaired users in monitoring pH levels without needing to read a display.

## Features

- Analog pH sensor input (connected to A0)
- 20-sample averaging for stable readings
- Voltage-to-pH conversion using calibration formula
- Automatic clamping of pH values to 0-14 range
- Audio feedback via DFPlayer Mini MP3 module
- Spoken pH announcement: "pH terbaca adalah [value]"
- 10-second interval between measurements
- Volume control for audio output

## Hardware Requirements

- Arduino Uno (ATmega328P) or compatible board
- Analog pH sensor with BNC connector
- DFPlayer Mini MP3 module
- MicroSD card (formatted FAT16/FAT32)
- Speaker or headphones (3W+ recommended)
- Jumper wires
- 10kΩ resistor (for pH sensor if needed)

## Software Requirements

- PlatformIO (for building and uploading)
- Arduino Framework
- Required Libraries:
  - DFRobotDFPlayerMini
  - SoftwareSerial (included with Arduino)
  - math.h (included with Arduino)

## Installation

1. Clone this repository
2. Install required libraries via PlatformIO:
   ```bash
   pio lib install
   ```
3. Prepare the MicroSD card:
   - Format as FAT16 or FAT32
   - Create MP3 files for numbers 0-9, "koma" (decimal point), and "pH terbaca adalah"
   - Name files according to the track mapping (see below)
4. Insert MicroSD card into DFPlayer Mini
5. Connect hardware according to pinout below
6. Build and upload using PlatformIO:
   ```bash
   pio run --target upload
   ```

## How It Works

1. **Sensor Reading**: The pH sensor outputs an analog voltage proportional to pH level
2. **Signal Conditioning**: 
   - 20 samples are taken and averaged to reduce noise
   - ADC value is converted to voltage (0-5V range)
3. **pH Calculation**: 
   - Voltage is converted to pH using: `pH = -35.458 * voltage + 171.26`
   - Result is clamped to 0-14 range (valid pH range)
4. **Audio Output**:
   - Integer and decimal parts of pH are separated
   - Corresponding MP3 tracks are played in sequence:
     - "pH terbaca adalah" (track 12)
     - Integer part
     - "koma" (track 11, decimal point)
     - Decimal part
   - Delays between tracks ensure clear separation

## Pinout

| Component | Pin | Arduino Pin |
|-----------|-----|-------------|
| pH Sensor | Signal | A0 (Analog) |
| pH Sensor | Power | 5V |
| pH Sensor | Ground | GND |
| DFPlayer Mini | TX | D11 (via SoftwareSerial) |
| DFPlayer Mini | RX | D10 (via SoftwareSerial) |
| DFPlayer Mini | VCC | 5V |
| DFPlayer Mini | GND | GND |
| Speaker | + | DFPlayer Mini SPK_1 |
| Speaker | - | DFPlayer Mini SPK_2 |

## MP3 Track Mapping

Place these MP3 files on your MicroSD card:

| Track Number | Filename | Content |
|--------------|----------|---------|
| 1 | 001.mp3 | "1" |
| 2 | 002.mp3 | "2" |
| 3 | 003.mp3 | "3" |
| 4 | 004.mp3 | "4" |
| 5 | 005.mp3 | "5" |
| 6 | 006.mp3 | "6" |
| 7 | 007.mp3 | "7" |
| 8 | 008.mp3 | "8" |
| 9 | 009.mp3 | "9" |
| 10 | 010.mp3 | "0" |
| 11 | 011.mp3 | "koma" (decimal point) |
| 12 | 012.mp3 | "pH terbaca adalah" |

## Calibration

The pH calculation uses the formula: `pH = -35.458 * voltage + 171.26`

You may need to adjust these coefficients based on your specific pH sensor:
1. Test with known pH solutions (4.0, 7.0, 10.0)
2. Measure the corresponding voltages
3. Calculate new slope and intercept for the linear equation

## Usage

1. Power on the Arduino
2. Wait for initialization message (via Serial Monitor if connected)
3. The system will announce "Sistem siap." when ready
4. Every 10 seconds:
   - Sensor is read and averaged
   - pH is calculated
   - Result is announced via speaker
5. Listen for the spoken pH value

## Notes

- Ensure adequate speaker volume for clear audio output
- The DFPlayer Mini may require a brief initialization delay
- For best results, use a quality pH sensor and calibrate regularly
- The system includes safety clamping to prevent unrealistic pH values (<0 or >14)
- Serial Monitor output is available for debugging (9600 baud)

## Troubleshooting

- **No audio**: Check MicroSD card format, file naming, and speaker connections
- **DFPlayer not found**: Verify wiring between Arduino and DFPlayer Mini
- **Unstable readings**: Increase sample count or add hardware filtering
- **Incorrect pH values**: Recalibrate using known buffer solutions
- **Garbled audio**: Check MP3 file quality and ensure proper spacing between tracks

## License

This project is open source and available for modification and redistribution.

## Built With

- PlatformIO
- Arduino Framework
- DFRobotDFPlayerMini Library

---

*Designed to assist visually impaired users in pH monitoring applications.*