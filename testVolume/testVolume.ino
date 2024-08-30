#include <Wire.h>

static const uint8_t indirectionVolume[128] PROGMEM = {
    0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120,
    4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
    2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
    6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
    1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
    5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
    3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
    7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127
};

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Wire.begin();
    Wire.beginTransmission(0x20);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.beginTransmission(0x21);
    Wire.write(0x00);
    Wire.endTransmission();
}

void loop() {
    // put your main code here, to run repeatedly:
    //setVolume(0x20, 0);
    setVolume(0x21, 0);
    for (uint8_t i = 0; i < 128; i++)
    {
        setVolume(0x20, 127 - i);
        setVolume(0x21, i);
        delay(100);
        Serial.print(i);
        Serial.print(" ");
        Serial.print(pgm_read_byte_near(indirectionVolume + i));
        Serial.print(" ");
        Serial.print(analogRead(A6));// * 444500 / 1024);
        Serial.print(" ");
        Serial.println(analogRead(A7));// * 444500 / 1024);
    }
    Serial.println("Fin");
    for(;;);
}

void setVolume(uint8_t adresse, uint8_t volume)
{
    Wire.beginTransmission(adresse);
    Wire.write(pgm_read_byte_near(indirectionVolume + volume) | 0x80);
    Wire.endTransmission();
}
