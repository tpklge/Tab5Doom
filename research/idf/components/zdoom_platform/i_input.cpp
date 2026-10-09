#include "d_event.h"
#include "d_gui.h"
#include "doomstat.h"
#include "c_console.h"
#include "menu/menu.h"
extern "C" int p4_doom_get_key(int *pressed, unsigned char *key);
static int scan(unsigned char key)
{
    switch(key) {
    case 0xad:return 0xc8; case 0xaf:return 0xd0; case 0xac:return 0xcb; case 0xae:return 0xcd;
    case 27:return 1; case 13:return 0x1c; case 9:return 0xf;
    case 0xa2:case ' ':return 0x39; case 0xa3:case 0x9d:return 0x1d;
    case 0xb6:return 0x2a; case 0xb8:return 0x38; case 0x7f:return 0xe;
    case '1':return 2;case '2':return 3;case '3':return 4;case '4':return 5;case '5':return 6;
    case '6':return 7;case '7':return 8;case '8':return 9;case '9':return 10;case '0':return 11;
    case 'w':return 0x11;case 'a':return 0x1e;case 's':return 0x1f;case 'd':return 0x20;
    case 'e':return 0x12;case 'q':return 0x10;case 'r':return 0x13;case 'f':return 0x21;
    case '-':return 0xc;case '=':return 0xd;
    default:if(key>=0xbb&&key<=0xc4)return key-0x80;return 0;
    }
}
void chex_poll_input()
{
    unsigned char key; int pressed;
    while(p4_doom_get_key(&pressed,&key)) {
        event_t event={};
        const bool gui = menuactive == MENU_On || menuactive == MENU_OnNoPause || (menuactive == MENU_Off && (ConsoleState == c_down || ConsoleState == c_falling));
        if(gui) {
            event.type=EV_GUI_Event;event.subtype=pressed?EV_GUI_KeyDown:EV_GUI_KeyUp;
            switch(key) {
            case 0xad:event.data1=GK_UP;break;case 0xaf:event.data1=GK_DOWN;break;
            case 0xac:event.data1=GK_LEFT;break;case 0xae:event.data1=GK_RIGHT;break;
            case 27:event.data1=GK_ESCAPE;break;case 13:event.data1=GK_RETURN;break;
            case 0xa2:event.data1=' ';break;default:event.data1=key<128?key:0;break;
            }
            event.data2=event.data1;
        } else {
            event.type=pressed?EV_KeyDown:EV_KeyUp;event.data1=scan(key);
            event.data2=key<128?key:0;
        }
        if(event.data1)D_PostEvent(&event);
    }
}
