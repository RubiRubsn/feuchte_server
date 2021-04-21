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
    EEPROM.write(0, byte1);
    EEPROM.write(1, byte2);
    EEPROM.write(2, byte3);
    EEPROM.write(3, byte4);
    for (int i = 0; i < 40; i++)
    {
        EEPROM.write(4 + i, dat.name[i]);
    }
    for (int i = 0; i < 40; i++)
    {
        EEPROM.write(44 + i, dat.SSID[i]);
    }
    for (int i = 0; i < 40; i++)
    {
        EEPROM.write(84 + i, dat.PSW[i]);
    }
    EEPROM.write(124, dat.reset);
    EEPROM.commit();
};

kali_dat Kalibrierung::laden()
{

    kali_dat dat;
    int byte1 = EEPROM.read(0);
    int byte2 = EEPROM.read(1);
    int byte3 = EEPROM.read(2);
    int byte4 = EEPROM.read(3);
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
    for (int i = 0; i < 40; i++)
    {
        dat.name[i] = EEPROM.read(4 + i);
    }
    for (int i = 0; i < 40; i++)
    {
        dat.SSID[i] = EEPROM.read(44 + i);
    }
    for (int i = 0; i < 40; i++)
    {
        dat.PSW[i] = EEPROM.read(84 + i);
    }
    dat.reset = EEPROM.read(124);
    return dat;
};
