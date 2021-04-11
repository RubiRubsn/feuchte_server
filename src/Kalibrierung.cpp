#include <Arduino.h>
#include <EEPROM.h>
#include "Kalibrierung.h"
#include <FS.h>

void Kalibrierung::speichern(kali_dat dat)
{
    int byte1 = dat.trocken >> 8;
    int byte2 = (dat.trocken & 0x00FF);
    int byte3 = dat.nass >> 8;
    int byte4 = (dat.nass & 0x00FF);
    EEPROM.write(1, byte1);
    EEPROM.write(2, byte2);
    EEPROM.write(3, byte3);
    EEPROM.write(4, byte4);
    EEPROM.commit();
};

kali_dat Kalibrierung::laden()
{

    kali_dat dat;
    int byte1 = EEPROM.read(1);
    int byte2 = EEPROM.read(2);
    int byte3 = EEPROM.read(3);
    int byte4 = EEPROM.read(4);
    dat.trocken = (byte1 << 8) + byte2;
    dat.nass = (byte3 << 8) + byte4;
    if (dat.trocken > 1025 || dat.trocken == 0)
    {
        dat.trocken = 1024;
    }
    if (dat.nass > 1025)
    {
        dat.nass = 300;
    }
    return dat;
};
