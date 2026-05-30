# STM32F401 Smart Lock Control System

## Multi-Modal Embedded Access Control

### Project Overview

This is a smart lock system I built using the STM32F401 Blackpill microcontroller. It lets you unlock a door three different ways - by typing a code on a keypad, tapping an RFID card, or using your phone via Bluetooth. The system also detects if someone tries to break it open.

### Features

- 4x4 Keypad for entering PIN code (you can change the password anytime)
- RC522 RFID reader for contactless cards (works at 13.56MHz)
- HC-05 Bluetooth module to unlock from your phone
- SW-420 vibration sensor that triggers alarm if someone shakes or hits the lock
- SG90 servo motor that acts as the lock (rotates to unlock)
- LCD screen that shows messages like "ACCESS GRANTED" or "WRONG ENTRY"
- Green LED for access granted, Red LED for denied or alarm
- Buzzer makes different sounds for grant, deny, and alarm

### Components Needed

| Component | Quantity |
|-----------|----------|
| STM32F401 Blackpill | 1 |
| SG90 Servo Motor | 1 |
| RC522 RFID Module | 1 |
| 4x4 Matrix Keypad | 1 |
| HC-05 Bluetooth Module | 1 |
| I2C LCD 16x2 | 1 |
| SW-420 Vibration Sensor | 1 |
| Breadboard | 2 |
| Jumper wires (M-M and M-F) | 60 pieces |
| Green LED + Red LED | 1 each |
| Active Buzzer | 1 |
| 220 ohm resistors | 2 |
| 100 ohm resistor | 1 |
| Breadboard power supply (5V/3.3V) | 1 |

### Pin Connections

| STM32 Pin | Connected To |
|-----------|--------------|
| PA0 | Green LED (through 220 ohm resistor) |
| PA1 | Red LED (through 220 ohm resistor) |
| PA2 | Buzzer (through 100 ohm resistor) |
| PA3 | Servo signal wire |
| PA4 | RFID chip select (CS) |
| PA5 | RFID clock (SCK) |
| PA6 | RFID MISO |
| PA7 | RFID MOSI |
| PB0 | RFID reset |
| PB8 | LCD SCL |
| PB9 | LCD SDA |
| PA9 | Bluetooth TX (to HC-05 RX) |
| PA10 | Bluetooth RX (to HC-05 TX) |
| PB12 | Keypad row 1 |
| PB13 | Keypad row 2 |
| PB14 | Keypad row 3 |
| PB15 | Keypad row 4 |
| PA8 | Keypad column 1 |
| PA9 | Keypad column 2 |
| PA10 | Keypad column 3 |
| PA11 | Keypad column 4 |
| 3.3V pin | RFID module VCC |
| 5V pin | Servo, LCD, Bluetooth, buzzer, LEDs |

### Code Files

- **main.c** - This is where everything runs. Main loop checks keypad, RFID, and Bluetooth flag. Also handles password change and access sequences.
- **i2c-lcd.c / i2c-lcd.h** - Driver for the LCD screen. Sends text and clear commands over I2C.
- **rc522.c / rc522.h** - Driver for the RFID module. Handles SPI communication and card reading.

### How Authentication Works

**Keypad:** User types 4 digits on the keypad. The code compares what was typed with the stored password. If match, unlock. If not, deny access. The '#' key lets you change the password - you have to enter the old one first.

**RFID:** When a card is placed near the RC522 reader, the module reads its unique ID (UID). The code compares this UID with the authorized card stored in memory. If it matches, unlock.

**Bluetooth:** The HC-05 module waits for incoming data over UART. When the phone sends the letter 'A', an interrupt triggers and sets a flag. The main loop sees this flag and unlocks the door.

### Tamper Detection

The SW-420 vibration sensor has a digital output. When it detects shaking or vibration (HIGH signal), the system immediately:

- Forces the servo to 0 degrees (locked position)
- Starts blinking the red LED every half second
- Turns on the buzzer continuously
- Shows "ALARM! TAMPER" on the LCD

The alarm does not stop until you reset the system by power cycling it.

### Power Setup

The system uses two voltage rails:

- **5V rail** powers the servo motor, LCD backlight, Bluetooth module, LEDs, and buzzer
- **3.3V rail** only powers the RFID module (it cannot handle 5V)

The servo motor has a 470uF capacitor across its power pins to prevent voltage drops when it starts moving.

### Built By

Mostafa Lotfy - 221004144
Shaden Ibrahim - 221004600
Filopater Nabil - 221005930

### License

This project was made for educational purposes as part of an embedded systems course.

### Notes

- The keypad had rows 3 and 4 swapped on my hardware, so I changed the lookup table in software instead of rewiring
- The servo needs the 220 ohm resistor on the signal line to protect the GPIO pin
- The LCD needs 4.7k ohm pull-up resistors on SCL and SDA if your backpack doesn't have them
- Keep the area under the RFID antenna clear of copper for best range (about 3-5cm)
