#include "FreqCountESP.h"
#include "settings.h"

bool read_TSL237(float &mySQMreading, double &nelm, double SQM_LIMIT)
{
  uint32_t frequency = -333; // measured TSL237 frequency which is dependent on light
  if (FreqCountESP.available())
  {
    frequency = FreqCountESP.read();
    if(frequency <= 0)
    {
      return false;
    }
    if (frequency < 1)
    {
      frequency = 1;
    }
    // 0.973 was derived by comparing TLS237 sensor readings against Unihedron and plotting on graph then deriving coefficient
    mySQMreading = (SQM_LIMIT - (2.5 * log10(frequency)) * 0.973); // frequency to magnitudes/arcSecond2
    if (isnan(mySQMreading))
    {
      mySQMreading = 0.0; // avoid nan and negative values
    }
    if (mySQMreading == -0.0)
    {
      mySQMreading = 0.0;
    }
    nelm = 7.93 - 5.0 * log10((pow(10, (4.316 - (mySQMreading / 5.0))) + 1));
    //irradiance = frequency / 2.3e3; // calculate irradiance as uW/(cm^2)
    return true;
  }
  else
  {
    return false;
  }
}

