#include <soc/sens_reg.h>
#include <soc/rtc.h>
#include <soc/timer_group_struct.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>
#include <driver/dac.h>

class AudioOutput;
void IRAM_ATTR timerInterrupt(AudioOutput *audioOutput);


class AudioOutput
{
  public:
  AudioSystem *audioSystem;
  int cleanRegDAC2;
  int cleanRegDAC1;
  
  void init(AudioSystem &audioSystem)
  {
    this->audioSystem = &audioSystem;
    timer_config_t config;
    config.alarm_en = 1;
    config.auto_reload = 1;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = 16;
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_en = TIMER_PAUSE;
    timer_init((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0, &config);
    timer_pause((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
    timer_set_counter_value((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0, 0x00000000ULL);
    timer_set_alarm_value((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0, 1.0/audioSystem.samplingRate * TIMER_BASE_CLK / config.divider);
    timer_enable_intr((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
    timer_isr_register((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0, (void (*)(void*))timerInterrupt, (void*) this, ESP_INTR_FLAG_IRAM, NULL);
    timer_start((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);

    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
    SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 255, RTC_IO_PDAC1_DAC_S);
    cleanRegDAC1 = (READ_PERI_REG(RTC_IO_PAD_DAC1_REG)&(~((RTC_IO_PDAC1_DAC)<<(RTC_IO_PDAC1_DAC_S))));
    cleanRegDAC2 = (READ_PERI_REG(RTC_IO_PAD_DAC2_REG)&(~((RTC_IO_PDAC2_DAC)<<(RTC_IO_PDAC2_DAC_S))));

    dac_output_enable((dac_channel_t)1);
  }

  void deinit()
  {
    timer_pause((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
  }

};

void IRAM_ATTR timerInterrupt(AudioOutput *audioOutput)
{
  uint32_t intStatus = TIMERG0.int_st_timers.val;
  if(intStatus & BIT(TIMER_0)) 
  {
    TIMERG0.hw_timer[TIMER_0].update = 1;
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[TIMER_0].config.alarm_en = 1;
    dac_output_voltage((dac_channel_t)1, audioOutput->audioSystem->nextSample());
    //WRITE_PERI_REG(RTC_IO_PAD_DAC2_REG, audioOutput->cleanRegDAC2 | ((audioOutput->audioSystem->nextSample() & RTC_IO_PDAC2_DAC) << RTC_IO_PDAC2_DAC_S));
  }
}  

