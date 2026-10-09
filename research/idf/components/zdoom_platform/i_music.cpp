// First bring-up deliberately uses the upstream null SFX renderer, no music.
#include "doomtype.h"
#include "i_music.h"
#include "c_cvars.h"
#include "m_joy.h"
int nomusic = 1;
CVAR(Float, snd_musicvolume, 0.0f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR(Float, spc_amp, 1.875f, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
void I_InitMusic() { nomusic = 1; }
void I_ShutdownMusic(bool) {}
void I_UpdateMusic() {}
void I_SetMusicVolume(float) {}
MusInfo *I_RegisterSong(FileReader *, MidiDeviceSetting *) { return nullptr; }
MusInfo *I_RegisterCDSong(int, int) { return nullptr; }
MusInfo *I_RegisterURLSong(const char *) { return nullptr; }
void MusInfo::Start(bool, float, int) {}
void I_BuildMIDIMenuList(FOptionValues *) {}
void I_BuildALDeviceList(FOptionValues *) {}
bool IsFModExPresent() { return false; }
bool IsOpenALPresent() { return false; }
void CD_Stop() {}
void CD_Pause() {}
void CD_Resume() {}
void CD_Eject() {}
void CD_UnEject() {}
void I_GetAxes(float axes[NUM_JOYAXIS]) { for (int i=0;i<NUM_JOYAXIS;++i) axes[i]=0; }
void I_GetJoysticks(TArray<IJoystickConfig *> &sticks) { sticks.Clear(); }
IJoystickConfig *I_UpdateDeviceList() { return nullptr; }
