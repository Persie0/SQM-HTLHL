#include "SparkFun_AS3935.h"
#include "settings.h"

#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// If you're using I-squared-C then keep the following line. Address is set to
// default.
SparkFun_AS3935 lightning(AS3935_ADDR);
// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector.
byte intVal = 0;


bool init_AS3935()
{
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightning_pin, INPUT);

  if (!lightning.begin())
  { // Initialize the sensor.
    
    return false;
  }
    
  // "Disturbers" are events that are false lightning events. If you find
  // yourself seeing a lot of disturbers you can have the chip not report those
  // events on the interrupt lines.
  // lightning.maskDisturber(true);

  lightning.setIndoorOutdoor(LOCATION);
  int enviVal = lightning.readIndoorOutdoor();

  lightning.setNoiseLevel(noiseFloor);

  int noiseVal = lightning.readNoiseLevel();

  lightning.watchdogThreshold(watchDogVal);

  int watchVal = lightning.readWatchdogThreshold();

  lightning.spikeRejection(spike);

  int spikeVal = lightning.readSpikeRejection();

  lightning.lightningThreshold(lightningThresh);

  uint8_t lightVal = lightning.readLightningThreshold();
  
  

  // When the distance to the storm is estimated, it takes into account other
  // lightning that was sensed in the past 15 minutes. If you want to reset
  // time, then you can call this function.

  // lightning.clearStatistics();

  // The power down function has a BIG "gotcha". When you wake up the board
  // after power down, the internal oscillators will be recalibrated. They are
  // recalibrated according to the resonance frequency of the antenna - which
  // should be around 500kHz. It's highly recommended that you calibrate your
  // antenna before using these two functions, or you run the risk of schewing
  // the timing of the chip.

  // lightning.powerDown();
  // delay(1000);
  // if( lightning.wakeUp() )
  //  
  // else
  // 

  // Set too many features? Reset them all with the following function.
  // lightning.resetSettings();
  return true;
}

void read_AS3935(int &lightning_distanceToStorm)
{
  if (digitalRead(lightning_pin) == HIGH)
  {
    // Hardware has alerted us to an event, now we read the interrupt register
    // to see exactly what it is.
    intVal = lightning.readInterruptReg();
    if (intVal == NOISE_INT)
    {
      
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      // lightning.setNoiseLevel(setNoiseLevel);
    }
    else if (intVal == DISTURBER_INT)
    {
      
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      // lightning.watchdogThreshold(threshVal);
    }
    else if (intVal == LIGHTNING_INT)
    {
      
      // Lightning! Now how far away is it? Distance estimation takes into
      // account any previously seen events in the last 15 seconds.
      lightning_distanceToStorm = lightning.distanceToStorm();
      
      
      
    }
  }
}
