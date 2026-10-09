#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <algorithm>
#include <string.h>
namespace ChexAudio {
static const unsigned OutputRate=22050;
struct Sample { std::vector<int16_t> pcm; unsigned rate=0,channels=0; int loop_start=0,loop_end=0; size_t frames() const { return channels?pcm.size()/channels:0; } };
inline uint16_t read16(const uint8_t *p) { return p[0]|(unsigned(p[1])<<8); }
inline uint32_t read32(const uint8_t *p) { return read16(p)|(uint32_t(read16(p+2))<<16); }
inline bool load_raw(Sample &sample,const uint8_t *data,size_t bytes,unsigned rate,unsigned channels,unsigned bits)
{
    if(!data||!bytes||!rate||rate>192000||(channels!=1&&channels!=2)||(bits!=8&&bits!=16))return false;
    size_t frame_bytes=channels*(bits/8);
    if(bytes%frame_bytes)return false;
    sample.rate=rate;sample.channels=channels;sample.pcm.resize(bytes/(bits/8));
    for(size_t n=0;n<sample.pcm.size();++n)
        sample.pcm[n]=bits==8?(int(data[n])-128)*256:int16_t(read16(data+n*2));
    sample.loop_end=sample.frames();return true;
}
inline bool load_wav(Sample &sample,const uint8_t *data,size_t bytes)
{
    if(!data||bytes<12||memcmp(data,"RIFF",4)||memcmp(data+8,"WAVE",4))return false;
    const uint64_t declared=uint64_t(read32(data+4))+8;
    if(declared>bytes||declared<12)return false;
    unsigned format=0,channels=0,rate=0,bits=0,align=0;
    const uint8_t *pcm=nullptr;size_t pcm_bytes=0;
    for(size_t offset=12;offset+8<=declared;) {
        const uint8_t *chunk=data+offset;uint32_t size=read32(chunk+4);offset+=8;
        if(size>declared-offset)return false;
        if(!memcmp(chunk,"fmt ",4)) {
            if(size<16)return false;
            format=read16(data+offset);channels=read16(data+offset+2);rate=read32(data+offset+4);
            align=read16(data+offset+12);bits=read16(data+offset+14);
        } else if(!memcmp(chunk,"data",4)){pcm=data+offset;pcm_bytes=size;}
        offset+=size;
        // Original Chex WAD omits padding on several final data chunks.
        if((size&1)&&offset<declared)++offset;
    }
    return format==1&&align==channels*(bits/8)&&load_raw(sample,pcm,pcm_bytes,rate,channels,bits);
}
struct Voice {
    Sample *sample=nullptr;uint64_t phase=0;unsigned step=0;
    int left=0,right=0;bool active=false,finished=false,loop=false,paused=false,nopause=false;
};
inline void mix(Voice *voices,size_t count,int16_t *output,size_t frames,bool pause,int gain)
{
    for(size_t n=0;n<frames;++n) {
        int64_t left=0,right=0;
        for(size_t c=0;c<count;++c) {
            Voice &voice=voices[c];
            if(!voice.active||voice.paused||(pause&&!voice.nopause))continue;
            const Sample &sample=*voice.sample;size_t frame=voice.phase>>16;
            size_t end=voice.loop?sample.loop_end:sample.frames();
            if(frame>=end) {
                if(voice.loop&&sample.loop_start<int(end)) {
                    const uint64_t start=uint64_t(sample.loop_start)<<16;
                    const uint64_t length=uint64_t(end-sample.loop_start)<<16;
                    voice.phase=start+(voice.phase-start)%length;frame=voice.phase>>16;
                } else {voice.active=false;voice.finished=true;continue;}
            }
            int l=sample.pcm[frame*sample.channels];int r=sample.channels==2?sample.pcm[frame*2+1]:l;
            left+=int64_t(l)*voice.left;right+=int64_t(r)*voice.right;
            voice.phase+=voice.step;
        }
        left=left*gain/(4096*4096);right=right*gain/(4096*4096);
        output[n*2]=int16_t(std::max<int64_t>(-32768,std::min<int64_t>(32767,left)));
        output[n*2+1]=int16_t(std::max<int64_t>(-32768,std::min<int64_t>(32767,right)));
    }
}
}
