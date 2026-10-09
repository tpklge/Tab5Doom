#include "hardware.h"
#include "v_palette.h"
#include "r_renderer.h"
#include "r_swrenderer.h"
#include "i_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>
#include <algorithm>
extern "C" void chex_present(const uint16_t *pixels);

class Tab5FrameBuffer : public DFrameBuffer
{
    DECLARE_CLASS(Tab5FrameBuffer, DFrameBuffer)
    PalEntry palette[256], flash;
    uint16_t colors[256], pixels[320*200];
    int flash_amount = 0;
    float gamma = 1.f;
    bool dirty = true;
public:
    Tab5FrameBuffer() : DFrameBuffer(320,200)
    { memcpy(palette,GPalette.BaseColors,sizeof(palette)); Accel2D=false; }
    bool Lock(bool buffered) override { return DSimpleCanvas::Lock(buffered); }
    void Update() override {
        if (LockCount != 1) { if (LockCount>1) --LockCount; return; }
        if (dirty) {
            for (int i=0;i<256;++i) {
                int r=(palette[i].r*(256-flash_amount)+flash.r*flash_amount)/256;
                int g=(palette[i].g*(256-flash_amount)+flash.g*flash_amount)/256;
                int b=(palette[i].b*(256-flash_amount)+flash.b*flash_amount)/256;
                r=std::min(255,(int)(pow(r/255.0,1.0/gamma)*255.0+0.5));
                g=std::min(255,(int)(pow(g/255.0,1.0/gamma)*255.0+0.5));
                b=std::min(255,(int)(pow(b/255.0,1.0/gamma)*255.0+0.5));
                colors[i]=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
            }
            dirty=false;
        }
        DrawRateStuff();
        for (int y=0;y<200;++y) for(int x=0;x<320;++x)
            pixels[y*320+x]=colors[Buffer[y*Pitch+x]];
        chex_present(pixels); DSimpleCanvas::Unlock();
    }
    PalEntry *GetPalette() override { return palette; }
    void GetFlashedPalette(PalEntry output[256]) override {
        for(int i=0;i<256;++i) {
            output[i]=palette[i];
            output[i].r=(palette[i].r*(256-flash_amount)+flash.r*flash_amount)/256;
            output[i].g=(palette[i].g*(256-flash_amount)+flash.g*flash_amount)/256;
            output[i].b=(palette[i].b*(256-flash_amount)+flash.b*flash_amount)/256;
        }
    }
    void UpdatePalette() override { dirty=true; }
    bool SetGamma(float value) override { gamma=std::max(0.1f,value); dirty=true; return true; }
    bool SetFlash(PalEntry rgb,int amount) override { flash=rgb; flash_amount=std::max(0,std::min(256,amount)); dirty=true; return true; }
    void GetFlash(PalEntry &rgb,int &amount) override { rgb=flash; amount=flash_amount; }
    int GetPageCount() override { return 1; }
    bool IsFullscreen() override { return true; }
};
IMPLEMENT_CLASS(Tab5FrameBuffer)
class Tab5Video : public IVideo
{
    bool offered=false;
public:
    EDisplayType GetDisplayType() override { return DISPLAY_FullscreenOnly; }
    void SetWindowedScale(float) override {}
    DFrameBuffer *CreateFrameBuffer(int,int,bool,DFrameBuffer *old) override {
        if (old) return old;
        auto result=new Tab5FrameBuffer;
        if (!result->IsValid()) I_FatalError("Unable to allocate Tab5 framebuffer");
        return result;
    }
    void StartModeIterator(int bits,bool) override { offered=bits!=8; }
    bool NextMode(int *w,int *h,bool *letterbox) override {
        if (offered) return false;
        offered=true; *w=320; *h=200; if(letterbox)*letterbox=false; return true;
    }
};
IVideo *Video=nullptr;
void I_CreateRenderer() { if (!Renderer) Renderer=new FSoftwareRenderer; }
void I_InitGraphics() { if (!Video) Video=new Tab5Video; atterm(I_ShutdownGraphics); }
void I_ShutdownGraphics() { delete screen; screen=nullptr; delete Video; Video=nullptr; }
DFrameBuffer *I_SetMode(int &w,int &h,DFrameBuffer *old)
{ w=320; h=200; if (!Video) I_InitGraphics(); return Video->CreateFrameBuffer(w,h,true,old); }
bool I_CheckResolution(int w,int h,int bpp) { return w==320 && h==200 && bpp==8; }
void I_ClosestResolution(int *w,int *h,int) { *w=320; *h=200; }
void I_SetFPSLimit(int) {}
void I_WaitVBL(int count) { if(count>0) vTaskDelay(pdMS_TO_TICKS(count*1000/70)+1); }
