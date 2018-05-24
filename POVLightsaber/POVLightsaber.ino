#include "Gyro.h"
#include "WS2812BI2S.h"
#include "AudioSystem.h"
#include "AudioOutput.h"
#include "sfx/sounds.h"
#include "SynthSound.h"

static const int buttonPin = 17;
static const int ledCount = 144;
static const int brightness = 128; 
static const int speed = 3;

unsigned char image[128*128][3];
int imageRes[] = {128, 128};
int currentImage = 0;

#include "File.h"

bool loadCurrentImage()
{
  char filename[32];
  sprintf(filename, "/spiffs/image%d.bin", currentImage);
  Serial.println(filename);
  if(!readFromFile(filename, image[0], 128 * 128 * 3))
  {
    for(int y = 0; y < 128; y++)
      for(int x = 0; x < 128; x++)
      {
        image[y * 128 + x][0] = 0;//x * 2;
        image[y * 128 + x][1] = 0;//y * 2;
        image[y * 128 + x][2] = 0;//254 - x * 2;
      }    
    return false;
  }
  return true;
}

bool saveCurrentImage()
{
  char filename[32];
  sprintf(filename, "/spiffs/image%d.bin", currentImage);
  Serial.println(filename);
  return writeToFile(filename, image[0], 128 * 128 * 3);
}

Gyro gyro(0, 1);
AudioSystem audioSystem(22050, 500);
AudioOutput audioOutput;
SynthSound *humSound = 0;

void setup()
{
  Serial.begin(115200);
  initPixels();
  //initialize audio output
  //audioOutput.init(audioSystem);
  pinMode(buttonPin, INPUT_PULLUP);
  gyro.calculateCorrection();
  initFileSystem();
  loadCurrentImage();
}

int pressed = 0;
bool on = false;
int visibleLeds = 0;
bool outputOn = false;

void turnOn()
{
  sounds.play(audioSystem, 0, 0.5, 1);  
  humSound = new SynthSound();
  humSound->init(audioSystem);
  audioSystem.play(humSound);
  on = true;
  visibleLeds = 0;
  gyro.wakeUp();
}

void turnOff()
{
  sounds.play(audioSystem, 1, 0.5, 1);  
  audioSystem.stop(humSound);
  humSound = 0;   
  on = false;
  visibleLeds = ledCount * speed + 100;
}

void loopSaber(int dt)
{
  static float angle = 0;
  gyro.poll();
  float td = sqrt(gyro.rotationV[0] * gyro.rotationV[0] + gyro.rotationV[1] * gyro.rotationV[1] + gyro.rotationV[2] * gyro.rotationV[2]);
  float d = gyro.rotationV[2] * dt * 0.001;
  angle += d;
  Serial.print(gyro.positionA[0]);
  Serial.print(" ");
  Serial.print(td);
  Serial.print(" ");
  Serial.println(angle);
 
  if(td < 5)
  {
    //standing still (correct angle)
    float l = sqrt(gyro.positionA[0] * gyro.positionA[0] + gyro.positionA[1] * gyro.positionA[1] + gyro.positionA[2] * gyro.positionA[2]);
    float rl = 1 / ((l == 0)? 1 : l);
    angle = angle * 0.9 + acos(rl * gyro.positionA[0]) * 180 / M_PI * 0.1;
  }

  int yi = (int)angle % imageRes[1];
  int p = imageRes[0] * yi;
  if(humSound)
  {
    float p = abs(gyro.rotationV[2] * 0.0001) + 1;
    if(p > 100)
      p = 100;
    p *= 256;
    humSound->pitch = (int)p;
    float v = gyro.rotationV[2] * gyro.rotationV[2] * 0.00001 + 0.5;
    if(v > 3)
      v = 3;
    v *= 256;
    float extrusion = (visibleLeds < 0) ? 0 : visibleLeds * 0.002f;
    if(extrusion > 1)
      extrusion = 1;
    v *= extrusion;
    humSound->volume = (int)v;
  }
  //x = towards button
  //y = towards volume
  //z = wield
  //Serial.println(gyro.positionA[1]);

  float sx = -cos(angle * M_PI / 180);
  float sy = -sin(angle * M_PI / 180);
  
/*  image[0][0] = 128;
  image[0][1] = 0;
  image[0][2] = 0;*/
  int sample = 0;
  for(int i = 0; i < pixelCount; i++)
  {
    int x = 64 + (int)(sx * (i + 20));
    int y = 150 + (int)(sy * (i + 20));
    if(i * speed < visibleLeds)
    {
      int a = 0;
      if(x >= 0 && y >= 0 && x < imageRes[0] && y < imageRes[1])
        a = imageRes[0] * y + x;
      pixels[sample++] = bitLUT[((int)image[a][1] * image[a][1]) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][0] * image[a][0]) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][2] * image[a][2]) >> 8];
      /*pixels[sample++] = bitLUT[((int)image[a][1] * brightness) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][0] * brightness) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][2] * brightness) >> 8];*/
      /*pixels[sample++] = bitLUT[image[a][1]];
      pixels[sample++] = bitLUT[image[a][0]];
      pixels[sample++] = bitLUT[image[a][2]];*/
    }
    else
    {
      pixels[sample++] = bitLUT[0];
      pixels[sample++] = bitLUT[0];
      pixels[sample++] = bitLUT[0];      
    }
  }   
  i2s_write_bytes(i2s_num, (char*)pixels, allocatedSamples * 4, portMAX_DELAY);  
}

void loop()
{
  static int time = 0;
  int t = millis();
  int dt = t - time;
  time = t;
  
  if(!digitalRead(buttonPin))
  {    
    pressed += dt;
    if(pressed > 1000)
    {
      if(on)
        turnOff();
      else
      {
        if(!outputOn)
        {
            audioOutput.init(audioSystem);
            outputOn = true;
        }
        turnOn();
      }
      pressed = -10000; 
    }
  }
  else
  {
    if(pressed > 100)
    {
      currentImage = (currentImage + 1) & 3;
      loadCurrentImage();
    }    
    pressed = 0;
  }
    
  loopSaber(dt);

  if(on)
    visibleLeds += dt;
  else
    visibleLeds -= dt;

  //fill audio buffer
  audioSystem.calcSamples();
}

