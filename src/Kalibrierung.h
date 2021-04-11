#include<Arduino.h>

struct kali_dat{
  int trocken = 0;
  int nass = 0;
};

class Kalibrierung{
  private:
  public:
    void speichern(kali_dat dat);
    kali_dat laden();
};

