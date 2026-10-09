#include "i_sound.h"
#include "files.h"
#include "i_system.h"
#include "pcm_mixer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <atomic>
#include <math.h>
extern "C" int chex_audio_open();
extern "C" int chex_audio_write(const short *,int);
extern "C" void chex_audio_close();
using ChexAudio::Sample;
class Tab5SoundRenderer : public SoundRenderer
{
    static const unsigned Count=16;
    ChexAudio::Voice voices[Count];
    FISoundChannel *handles[Count]={};
    float volumes[Count]={};
    SoundListener listener={};
    SemaphoreHandle_t mutex=nullptr,done=nullptr;
    std::atomic<bool> running{false};
    bool valid=false;unsigned pauses=0;int master=4096;
    EInactiveState inactive=INACTIVE_Active;
    void lock(){xSemaphoreTake(mutex,portMAX_DELAY);}
    void unlock(){xSemaphoreGive(mutex);}
    static void worker(void *argument)
    {
        auto self=static_cast<Tab5SoundRenderer *>(argument);
        int16_t output[256*2];unsigned errors=0;
        while(self->running.load()) {
            self->lock();
            ChexAudio::mix(self->voices,Count,output,256,
                self->pauses||self->inactive==INACTIVE_Complete,
                self->inactive==INACTIVE_Mute?0:self->master);
            self->unlock();
            int result=chex_audio_write(output,sizeof(output));
            if(result){if(errors++<5)ESP_LOGE("CHEX_AUDIO","I2S write failed: %d",result);vTaskDelay(pdMS_TO_TICKS(20));}
        }
        xSemaphoreGive(self->done);vTaskDelete(nullptr);
    }
    int index(FISoundChannel *channel)
    {
        if(!channel||!channel->SysChannel)return -1;
        for(unsigned n=0;n<Count;++n)if(channel->SysChannel==&voices[n]&&handles[n]==channel)return n;
        return -1;
    }
    void gains(unsigned n,float volume,float pan)
    {
        volume=std::max(0.f,std::min(1.f,volume));pan=std::max(-1.f,std::min(1.f,pan));
        voices[n].left=int(volume*4096*(pan>0?1-pan:1));
        voices[n].right=int(volume*4096*(pan<0?1+pan:1));
    }
    void spatial(unsigned n,SoundListener *ears,FRolloffInfo *rolloff,float scale,const FVector3 &position)
    {
        if(!ears||!ears->valid){gains(n,volumes[n],0);return;}
        float dx=position.X-ears->position.X,dy=position.Y-ears->position.Y,dz=position.Z-ears->position.Z;
        float distance=sqrtf(dx*dx+dy*dy+dz*dz)*scale;
        float attenuation=rolloff?S_GetRolloff(rolloff,distance,false):1.f;
        float planar=sqrtf(dx*dx+dy*dy);
        float pan=planar>0.01f?(sinf(ears->angle)*dx-cosf(ears->angle)*dy)/planar:0;
        gains(n,volumes[n]*attenuation,pan);
    }
public:
    Tab5SoundRenderer()
    {
        mutex=xSemaphoreCreateMutex();done=xSemaphoreCreateBinary();
        if(!mutex||!done)return;
        if(chex_audio_open()!=0){ESP_LOGE("CHEX_AUDIO","Codec initialization failed");chex_audio_close();return;}
        running.store(true);
        if(xTaskCreatePinnedToCore(worker,"chex_audio",8192,this,5,nullptr,1)!=pdPASS){running.store(false);chex_audio_close();return;}
        valid=true;ESP_LOGI("CHEX_AUDIO","16 SFX voices; WAV/raw PCM8/16; music disabled");
    }
    ~Tab5SoundRenderer() override
    {
        if(running.exchange(false)){xSemaphoreTake(done,portMAX_DELAY);chex_audio_close();}
        if(mutex)vSemaphoreDelete(mutex);
        if(done)vSemaphoreDelete(done);
    }
    bool IsValid() override{return valid;}
    void SetSfxVolume(float value) override{lock();master=int(std::max(0.f,std::min(1.f,value))*4096);unlock();}
    void SetMusicVolume(float) override{}
    SoundHandle LoadSoundRaw(BYTE *data,int length,int rate,int channels,int bits,int start,int end=-1) override
    {
        SoundHandle result={nullptr};if(length<=0||rate<=0||channels<=0||bits<=0)return result;
        Sample *sample=new Sample;
        if(!ChexAudio::load_raw(*sample,data,length,rate,channels,bits)){delete sample;return result;}
        sample->loop_start=start<0?0:std::min<int>(start,sample->frames()-1);
        sample->loop_end=end<=sample->loop_start?sample->frames():std::min<int>(end,sample->frames());
        result.data=sample;return result;
    }
    SoundHandle LoadSound(BYTE *data,int length) override
    {
        SoundHandle result={nullptr};if(length<=0)return result;
        Sample *sample=new Sample;
        if(!ChexAudio::load_wav(*sample,data,length)){delete sample;ESP_LOGW("CHEX_AUDIO","Unsupported/corrupt SFX (%d bytes)",length);return result;}
        result.data=sample;
        static unsigned logged=0;
        if(logged++<5)ESP_LOGI("CHEX_AUDIO","WAV: %u Hz, %u channels, %u frames",sample->rate,sample->channels,(unsigned)sample->frames());
        return result;
    }
    void UnloadSound(SoundHandle sound) override
    {
        auto sample=static_cast<Sample *>(sound.data);if(!sample)return;
        for(unsigned n=0;n<Count;++n){lock();auto h=voices[n].sample==sample?handles[n]:nullptr;unlock();if(h)StopChannel(h);}
        delete sample;
    }
    unsigned GetMSLength(SoundHandle sound) override{auto s=static_cast<Sample *>(sound.data);return s?uint64_t(s->frames())*1000/s->rate:0;}
    unsigned GetSampleLength(SoundHandle sound) override{auto s=static_cast<Sample *>(sound.data);return s?s->frames():0;}
    float GetOutputRate() override{return ChexAudio::OutputRate;}
    SoundStream *CreateStream(SoundStreamCallback,int,int,int,void *) override{return nullptr;}
    SoundStream *OpenStream(FileReader *reader,int) override{delete reader;return nullptr;}
    FISoundChannel *StartSound(SoundHandle sound,float volume,int pitch,int flags,FISoundChannel *reuse) override
    {
        if(!sound.data)return nullptr;
        UpdateSounds();lock();int selected=-1;
        for(unsigned n=0;n<Count;++n)if(!handles[n]){selected=n;break;}
        if(selected<0){unlock();return nullptr;}
        unsigned n=selected;auto sample=static_cast<Sample *>(sound.data);
        voices[n]=ChexAudio::Voice();voices[n].sample=sample;
        voices[n].step=std::max<uint64_t>(1,uint64_t(sample->rate)*std::max(1,std::min(255,pitch))*65536/(128*ChexAudio::OutputRate));
        voices[n].loop=flags&SNDF_LOOP;voices[n].nopause=flags&SNDF_NOPAUSE;volumes[n]=volume;
        handles[n]=reuse?reuse:S_GetChannel(&voices[n]);handles[n]->SysChannel=&voices[n];
        handles[n]->Rolloff.RolloffType=0;handles[n]->DistanceScale=1;handles[n]->DistanceSqr=0;handles[n]->ManualRolloff=false;
        gains(n,volume,0);voices[n].active=true;
        auto handle=handles[n];unlock();return handle;
    }
    FISoundChannel *StartSound3D(SoundHandle s,SoundListener *ears,float volume,FRolloffInfo *rolloff,float scale,int pitch,int,const FVector3 &position,const FVector3 &,int,int flags,FISoundChannel *reuse) override
    {
        auto handle=StartSound(s,volume,pitch,flags,reuse);if(!handle)return nullptr;
        lock();int n=index(handle);if(n>=0){if(rolloff)handle->Rolloff=*rolloff;handle->DistanceScale=scale;spatial(n,ears,rolloff,scale,position);}unlock();return handle;
    }
    void StopChannel(FISoundChannel *handle) override
    {
        lock();int n=index(handle);if(n<0){unlock();return;}
        voices[n].active=false;voices[n].finished=false;unlock();
        S_ChannelEnded(handle);lock();handles[n]=nullptr;voices[n].sample=nullptr;unlock();
    }
    void ChannelVolume(FISoundChannel *handle,float value) override{lock();int n=index(handle);if(n>=0){volumes[n]=value;gains(n,value,0);}unlock();}
    void MarkStartTime(FISoundChannel *handle) override{if(handle)handle->StartTime.AsOne=0;}
    unsigned GetPosition(FISoundChannel *handle) override{lock();int n=index(handle);unsigned position=n>=0?voices[n].phase>>16:0;unlock();return position;}
    float GetAudibility(FISoundChannel *handle) override{lock();int n=index(handle);float value=n>=0?float(std::max(voices[n].left,voices[n].right))*master/(4096*4096):0;unlock();return value;}
    void Sync(bool) override{}
    void SetSfxPaused(bool value,int slot) override{if(slot<0||slot>=32)return;lock();if(value)pauses|=1u<<slot;else pauses&=~(1u<<slot);unlock();}
    void SetInactive(EInactiveState value) override{lock();inactive=value;unlock();}
    void UpdateSoundParams3D(SoundListener *ears,FISoundChannel *handle,bool,const FVector3 &position,const FVector3 &) override{lock();int n=index(handle);if(n>=0)spatial(n,ears,&handle->Rolloff,handle->DistanceScale,position);unlock();}
    void UpdateListener(SoundListener *ears) override{if(ears){lock();listener=*ears;unlock();}}
    void UpdateSounds() override
    {
        for(unsigned n=0;n<Count;++n){lock();auto handle=voices[n].finished?handles[n]:nullptr;unlock();if(handle)StopChannel(handle);}
    }
    void PrintStatus() override{Printf("Tab5 ES8388: 22050 Hz stereo, 16 SFX voices\n");}
    void PrintDriversList() override{Printf("tab5: ES8388/I2S\n");}
};
SoundRenderer *CreateTab5SoundRenderer(){return new Tab5SoundRenderer;}
