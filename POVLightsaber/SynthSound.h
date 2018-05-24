#pragma once

class SynthSound: public SoundBase
{
  public:
  unsigned int position[5];
  int positionIncrement[5];
  signed char sinTab[256];
  int pitch;
  int volume;
    
  void init(AudioSystem &audioSystem)
  {
    static const int baseFreq[] = {74, 92, 278, 370, 462};
    next = 0;
    id = 0;
    pitch = 256;
    for(int i = 0; i < 5; i++)
    {
      position[i] = 0;
      positionIncrement[i] = int((65536.0 * 256.0 * baseFreq[i]) / audioSystem.samplingRate);
    }
    this->volume = 1 * 256;
    playing = true;
    for(int i = 0; i < 256; i++)
      sinTab[i] = int(sin((i / 128.) * M_PI) * 127);
  }

  virtual int nextSample()
  {
    static const int baseVolume[] = {64, 170, 90, 50, 40};
    int s = 0;
    for(int i = 0; i < 5; i++)
    {
      s += (sinTab[(position[i] >> 16) & 255] * baseVolume[i] * volume) >> 8;
      position[i] += (positionIncrement[i] * pitch) >> 8;
    }
    return s;
  }
};
