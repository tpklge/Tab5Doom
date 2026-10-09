"""Exercise the actual PCM loader/mixer and every official Chex 3 DS lump."""
from pathlib import Path
import struct, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
harness=r'''
#include "pcm_mixer.h"
#include <assert.h>
#include <fstream>
#include <iterator>
using namespace ChexAudio;
int main(int argc,char **argv) {
 Sample s;uint8_t raw[]={128,255,0};assert(load_raw(s,raw,3,11025,1,8));
 assert(s.pcm[0]==0&&s.pcm[1]==32512&&s.pcm[2]==-32768);
 assert(!load_raw(s,raw,3,11025,2,16));assert(!load_raw(s,raw,3,0,1,8));
 assert(!load_raw(s,raw,3,200000,1,8));
 Voice v;v.sample=&s;v.active=true;v.step=32768;v.left=v.right=4096;
 int16_t out[16];mix(&v,1,out,8,false,4096);
 assert(out[0]==0&&out[4]==32512&&out[8]==-32768&&out[12]==0&&v.finished);
 v=Voice();v.sample=&s;v.active=true;v.step=65536;v.left=v.right=4096;
 mix(&v,1,out,2,true,4096);assert(v.phase==0&&out[0]==0);
 v.nopause=true;mix(&v,1,out,1,true,4096);assert(v.phase==65536);
 v.loop=true;s.loop_start=1;s.loop_end=3;mix(&v,1,out,8,false,4096);
 assert(out[0]==32512&&out[2]==-32768&&out[4]==32512&&v.active);
 Voice both[2]={v,v};mix(both,2,out,1,false,4096);assert(out[0]==32767);
 uint8_t stereo[]={0xff,0x7f,0,0x80};Sample st;
 assert(load_raw(st,stereo,4,44100,2,16));
 v=Voice();v.sample=&st;v.active=true;v.step=131072;v.left=v.right=4096;
 mix(&v,1,out,2,false,2048);assert(out[0]==16383&&out[1]==-16384&&v.finished);
 for(int n=1;n<argc;++n) {
  std::ifstream file(argv[n],std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),{});
  Sample wav;assert(load_wav(wav,data.data(),data.size()));assert(wav.frames()>0);
  assert(wav.channels==1&&(wav.rate==11025||wav.rate==44100));
  assert(!load_wav(wav,data.data(),11));
  assert(!load_wav(wav,data.data(),data.size()-1));
  data[4]=data[5]=data[6]=data[7]=255;assert(!load_wav(wav,data.data(),data.size()));
 }
}
'''
with tempfile.TemporaryDirectory() as folder:
 p=Path(folder);source=p/'test.cpp';source.write_text(harness)
 subprocess.run(['c++','-std=c++11','-fsanitize=address,undefined','-I',str(root/'research/idf/components/zdoom_platform'),str(source),'-o',str(p/'test')],check=True)
 wad=(root/'research/assets/chex3.wad').read_bytes();count,offset=struct.unpack_from('<II',wad,4);sounds=[]
 for n in range(count):
  pos,size,name=struct.unpack_from('<II8s',wad,offset+n*16)
  if name.startswith(b'DS'):
   f=p/(name.rstrip(b'\0').decode()+'.wav');f.write_bytes(wad[pos:pos+size]);sounds.append(str(f))
 assert len(sounds)==84,len(sounds)
 subprocess.run([str(p/'test'),*sounds],check=True)
print('Audio: 84 official SFX + resampling, loops, pause, clipping, stereo, corrupt WAV: PASS (ASan/UBSan)')
