#include "driver/i2s.h"

static const i2s_port_t i2s_num = (i2s_port_t)1;
static const int pixelCount = 144 + 2;
static const int samplesNeeded = (pixelCount * 24 * 4 + 31) / 32;
static const int stereoSamplesNeeded = (samplesNeeded + 1) / 2;
static const int bufferSize = 128;
static const int bufferCount = (stereoSamplesNeeded + bufferSize - 1) / bufferSize;
static const int allocatedSamples = bufferCount * bufferSize * 2;

static unsigned int bitLUT[256];

static const i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
     .sample_rate = 833333,
     .bits_per_sample = (i2s_bits_per_sample_t)32,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT ,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = 0,
     .dma_buf_count = bufferCount * 2,
     .dma_buf_len = bufferSize,
     .use_apll = false
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = 12,
    .ws_io_num = 14,
    .data_out_num = 27,
    .data_in_num = I2S_PIN_NO_CHANGE
};

unsigned long pixels[allocatedSamples];

void initPixels() 
{
    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    i2s_set_pin(i2s_num, &pin_config);
    i2s_set_sample_rates(i2s_num, 833333);
    
    //this is the hack that enables the highest sampling rate possible ~13MHz, have fun
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(1), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(1), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(1), I2S_CLKM_DIV_NUM_V, 18, I2S_CLKM_DIV_NUM_S); 
    SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(1), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);

  for(int i = 0; i < allocatedSamples; i++)
    pixels[i] = 0;
  for(int i = 0; i < 256; i++)
  {
    bitLUT[i] = 0;
    for(int bit = 7; bit >= 0; bit--)
      bitLUT[i] |= (((i >> bit) & 1)?0b1110 : 0b1000) << (bit * 4);
  }
}

void loopPixels() 
{
  static int rgb = 0;
  rgb++;
  int sample = 0;
  for(int i = 0; i < pixelCount; i++)
  {
    pixels[sample++] = bitLUT[(rgb >> 8) & 255];
    pixels[sample++] = bitLUT[rgb & 255];
    pixels[sample++] = bitLUT[(rgb >> 16) & 255];
  }   
  i2s_write_bytes(i2s_num, (char*)pixels, allocatedSamples * 4, portMAX_DELAY);
}
