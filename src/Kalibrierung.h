#include <Arduino.h>

struct kali_dat
{
  int trocken = 0;
  int nass = 0;
  char SSID[40];
  char PSW[40];
  char name[40];
  uint8_t reset;
};

class Kalibrierung
{
private:
public:
  void speichern(kali_dat dat);
  kali_dat laden();
};
