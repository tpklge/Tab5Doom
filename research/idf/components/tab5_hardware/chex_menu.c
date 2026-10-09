#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "doomkeys.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define WIDTH 320
#define HEIGHT 200
extern void chex_present(const uint16_t *);
extern int p4_doom_get_key(int *,unsigned char *);
static const char *names[]={"CHEX QUEST 1","CHEX QUEST 2","CHEX QUEST 3"};
static const char *files[]={"/sdcard/doom/chex.wad","/sdcard/doom/chex2.wad","/sdcard/doom/chex3.wad"};
static uint32_t le32(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool valid_wad(const char *path)
{
 FILE *f=fopen(path,"rb");if(!f)return false;
 uint8_t h[12];bool ok=fread(h,1,12,f)==12;
 if(ok)ok=!memcmp(h,"IWAD",4)||!memcmp(h,"PWAD",4);
 if(ok){fseek(f,0,SEEK_END);long size=ftell(f);ok=size>=12&&le32(h+4)>0&&le32(h+8)>=12&&(uint64_t)le32(h+8)+(uint64_t)le32(h+4)*16<=(uint64_t)size;}
 fclose(f);return ok;
}
/* Original compact 5x7 font: A-Z, then 0-9. Rows use the low five bits. */
static const uint8_t glyphs[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
};

static void text(uint16_t *frame, int x, int y, const char *s,
                 int scale, uint16_t color)
{
    for (; *s; s++, x += 6 * scale) {
        const uint8_t *glyph = NULL;
        if (*s >= 'A' && *s <= 'Z') glyph = glyphs[*s - 'A'];
        if (*s >= '0' && *s <= '9') glyph = glyphs[26 + *s - '0'];
        for (int row = 0; row < 7; row++) {
            uint8_t bits = glyph ? glyph[row] : 0;
            if (*s == '.' && row == 6) bits = 4;
            if (*s == '/' && row < 5) bits = 1 << row;
            if (*s == '-' && row == 3) bits = 14;
            for (int col = 0; col < 5; col++) {
                if (!(bits & (1 << (4 - col)))) continue;
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++) {
                        int px = x + col * scale + dx;
                        int py = y + row * scale + dy;
                        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
                            frame[py * WIDTH + px] = color;
                    }
            }
        }
    }
}


static void refresh(bool ready[3])
{
 bool base=valid_wad(files[0]);ready[0]=base;ready[1]=base&&valid_wad(files[1]);ready[2]=valid_wad(files[2]);
 for(int i=0;i<3;++i)ESP_LOGI("CHEX_MENU","%s: %s",names[i],ready[i]?"ready":"missing/invalid WAD");
}
static void render(uint16_t *frame,int selected,const bool ready[3],const char *message)
{
 memset(frame,0,WIDTH*HEIGHT*sizeof(*frame));text(frame,106,8,"TAB5 CHEX",2,0xffe0);
 for(int i=0;i<3;++i){int y=40+i*36;
  if(i==selected)for(int row=y-4;row<y+29;++row)for(int col=10;col<310;++col)frame[row*WIDTH+col]=0x18c3;
  text(frame,24,y,names[i],2,ready[i]?0xffff:0x8410);
  text(frame,24,y+18,i==1?"CHEX.WAD + CHEX2.WAD":i==0?"CHEX.WAD":"CHEX3.WAD",1,0x07ff);
  text(frame,244,y+18,ready[i]?"PRONTO":"AUSENTE",1,ready[i]?0x07e0:0xf800);
 }
 text(frame,12,154,message,1,0xffe0);
 text(frame,12,173,"SETAS OU 1/2/3 - ENTER PARA JOGAR",1,0xffff);
 text(frame,12,186,"R - BUSCAR WADS NA PASTA /DOOM",1,0x07ff);
}
int chex_select_game(void)
{
 uint16_t *frame=heap_caps_malloc(WIDTH*HEIGHT*sizeof(*frame),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 if(!frame)return -1;
 bool ready[3];refresh(ready);int selected=ready[0]?0:ready[1]?1:2;
 const char *message="ESCOLHA O JOGO";bool dirty=true;
 for(;;){
  if(dirty){render(frame,selected,ready,message);chex_present(frame);dirty=false;}
  int pressed;unsigned char key;
  if(!p4_doom_get_key(&pressed,&key)||!pressed){vTaskDelay(pdMS_TO_TICKS(10));continue;}
  if(key==KEY_UPARROW||key=='w'){selected=(selected+2)%3;dirty=true;}
  else if(key==KEY_DOWNARROW||key=='s'){selected=(selected+1)%3;dirty=true;}
  else if(key>='1'&&key<='3'){selected=key-'1';dirty=true;}
  else if(key=='r'||key=='R'){refresh(ready);message="BUSCA CONCLUIDA";dirty=true;}
  else if(key==KEY_ENTER){
   if(!ready[selected]){message="WAD AUSENTE OU INVALIDO - USE R";dirty=true;continue;}
   // Consume queued menu releases before the engine starts.
   while(p4_doom_get_key(&pressed,&key)){}
   heap_caps_free(frame);ESP_LOGI("CHEX_MENU","Selected: %s",names[selected]);return selected;
  }
 }
}
