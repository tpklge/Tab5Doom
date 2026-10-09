#include "bsp_tab5.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
static esp_codec_dev_handle_t speaker;
int chex_audio_open(void)
{
    bsp_audio_init(NULL);
    speaker=bsp_audio_codec_speaker_init();
    if(!speaker)return -1;
    esp_codec_dev_sample_info_t format={.sample_rate=22050,.channel=2,.bits_per_sample=16};
    int result=esp_codec_dev_open(speaker,&format);
    if(result!=ESP_CODEC_DEV_OK)return result;
    if(bsp_audio_configure_output()!=ESP_OK)return -1;
    if(esp_codec_dev_set_out_vol(speaker,70)!=ESP_CODEC_DEV_OK)return -1;
    result=esp_codec_dev_set_out_mute(speaker,false);
    ESP_LOGI("CHEX_AUDIO","ES8388 ready: 22050 Hz stereo PCM16, volume 70");
    return result;
}
int chex_audio_write(const short *samples,int bytes)
{ return esp_codec_dev_write(speaker,(void *)samples,bytes); }
void chex_audio_close(void)
{
    if(speaker) { esp_codec_dev_set_out_mute(speaker,true);esp_codec_dev_close(speaker);esp_codec_dev_delete(speaker);speaker=NULL; }
}
