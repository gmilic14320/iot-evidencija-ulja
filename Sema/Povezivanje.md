# Povezivanje

ESP32-S3, HC-SR04 i LCD 1602A sa I2C dodatkom. Sve povezivati bez napajanja.

| HC-SR04 | ESP32 |
|---|---|
| VCC | 5V |
| GND | GND |
| TRIG | IO4 |
| ECHO | IO5 preko otpornika ispod |

    ECHO -- [1 kΩ] --+-- [1 kΩ] -- [1 kΩ] -- GND
                    |
                   IO5

ECHO ne povezivati direktno na ESP32.

| LCD I2C | ESP32 |
|---|---|
| GND | GND |
| VCC | 3V3 |
| SDA | IO6 |
| SCL | IO7 |

Na našem LCD dodatku adresa je 0x27. Za napajanje LCD-a sa 5V potreban je odgovarajući I2C pretvarač nivoa.

Fotografije: [senzor](Fotografije/01-senzor.jpg), [ESP32](Fotografije/02-esp32.jpg), [LCD](Fotografije/03-lcd.jpg).
