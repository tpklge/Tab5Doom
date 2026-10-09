#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_err.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "doomerrors.h"
#include "c_console.h"
#include <exception>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/stat.h>
#include <stdio.h>
extern void D_DoomMain();
extern "C" esp_err_t chex_board_start();
extern "C" int chex_compression_probe();
extern "C" int chex_select_game();
extern void chex_run_terminators();
static bool check_file(const char *path)
{
    FILE *file=fopen(path,"rb");
    if(!file){ ESP_LOGE("CHEX","Required file missing: %s",path);return false; }
    fclose(file);return true;
}
extern "C" void app_main()
{
    ESP_LOGI("CHEX","ZDoom 2.8.1 / Tab5 experimental bring-up; SFX audio enabled");
    if(chex_board_start()!=ESP_OK) { ESP_LOGE("CHEX","Hardware/SD initialization failed");return; }
    if(!check_file("/sdcard/doom/zdoom.pk3")) return;
    int game=chex_select_game();if(game<0){ESP_LOGE("CHEX","Menu allocation failed");return;}
    const char *configs[]={"/sdcard/doom/chex1-zdoom.ini","/sdcard/doom/chex2-zdoom.ini","/sdcard/doom/chex3-zdoom.ini"};
    const char *saves[]={"/sdcard/doom/chex1-saves/","/sdcard/doom/chex2-saves/","/sdcard/doom/chex3-saves/"};
    mkdir(saves[game],0777);
    int compression=chex_compression_probe();
    if(compression) { ESP_LOGE("CHEX","Compression probe failed: %d",compression);return; }
    ESP_LOGI("CHEX","RAM before startup: internal=%u PSRAM=%u largest=%u",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    try {
        progdir="/sdcard/doom/";
        const char *arguments[24];int count=0;
        arguments[count++]="/sdcard/doom/tab5chex";
        arguments[count++]="-iwad";arguments[count++]=game==2?"/sdcard/doom/chex3.wad":"/sdcard/doom/chex.wad";
        if(game==1){arguments[count++]="-file";arguments[count++]="/sdcard/doom/chex2.wad";}
        arguments[count++]="-config";arguments[count++]=configs[game];
        arguments[count++]="-savedir";arguments[count++]=saves[game];
        arguments[count++]="-nomusic";arguments[count++]="-noautoload";
        arguments[count++]="-width";arguments[count++]="320";
        arguments[count++]="-height";arguments[count++]="200";
        arguments[count++]="+map";arguments[count++]="E1M1";
        Args=new DArgs(count,const_cast<char **>(arguments));
        ESP_LOGI("CHEX","Starting original Chex Quest %d, E1M1",game+1);
        C_InitConsole(320,200,false);
        D_DoomMain();
    } catch(const CDoomError &error) {
        ESP_LOGE("CHEX","Engine stopped: %s",error.GetMessage()?error.GetMessage():"normal exit");
    } catch(const std::exception &error) {
        ESP_LOGE("CHEX","C++ exception: %s",error.what());
    } catch(...) { ESP_LOGE("CHEX","Unknown engine exception"); }
    ESP_LOGI("CHEX","Engine stopped; reboot required before retry");
    while(true) vTaskDelay(pdMS_TO_TICKS(1000));
}
