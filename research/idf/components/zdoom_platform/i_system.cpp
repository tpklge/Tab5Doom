#include "esp_log.h"
#include "i_system.h"
#include "doomerrors.h"
#include "m_argv.h"
#include "d_ticcmd.h"
#include "d_net.h"
#include "doomstat.h"
#include "st_start.h"
#include "x86.h"
#include "c_cvars.h"
#include "i_sound.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <sys/stat.h>
#include <string.h>

DArgs *Args = nullptr;
extern "C" { CPUInfo CPU = {}; }
DWORD LanguageIDs[4];
bool gameisdead = false;
FStartupScreen *StartScreen = nullptr;
static std::vector<void (*)()> terminators;
extern void I_InitTimer();
extern void I_ShutdownTimer();
extern void chex_poll_input();
extern "C" void chex_prepare_restart();
void addterm(void (*func)(), const char *) { terminators.push_back(func); }
void popterm() { if (!terminators.empty()) terminators.pop_back(); }
void chex_run_terminators()
{
    while (!terminators.empty()) {
        auto func = terminators.back(); terminators.pop_back(); func();
    }
}
EXTERN_CVAR(String, snd_backend)
void I_Init() { I_InitTimer(); snd_backend="tab5"; I_InitSound(); atterm(I_ShutdownSound); }
void I_Quit()
{
    ESP_LOGI("CHEX", "Quit requested; shutting down engine");
    // Hide panel transitions before video/audio shutdown callbacks.
    chex_prepare_restart();
    chex_run_terminators();
    ESP_LOGI("CHEX", "Shutdown complete; restarting to game selector");
    esp_restart();
}
void I_FatalError(const char *format, ...)
{
    char message[MAX_ERRORTEXT]; va_list args; va_start(args, format);
    vsnprintf(message, sizeof(message), format, args); va_end(args);
    gameisdead = true; fprintf(stderr, "CHEX FATAL: %s\n", message);
    throw CFatalError(message);
}
void I_Error(const char *format, ...)
{
    char message[MAX_ERRORTEXT]; va_list args; va_start(args, format);
    vsnprintf(message, sizeof(message), format, args); va_end(args);
    throw CRecoverableError(message);
}
void I_PrintStr(const char *text)
{
    for (; *text; ++text) {
        const unsigned char c = *text;
        if (c >= 0x1c && c <= 0x1f) { if (text[1]) ++text; }
        else fputc(c, stdout);
    }
    fflush(stdout);
}
void SetLanguageIDs() { for (auto &id : LanguageIDs) id = MAKE_ID('e','n','u',0); }
unsigned int I_MakeRNGSeed() { return esp_random(); }
void I_StartFrame() {}
void I_StartTic() { chex_poll_input(); }
ticcmd_t *I_BaseTiccmd() { static ticcmd_t empty = {}; return &empty; }
void I_Tactile(int, int, int) {}
void I_SetIWADInfo() {}
int I_PickIWad(WadStuff *, int count, bool, int preferred)
{ return count > 0 ? (preferred >= 0 && preferred < count ? preferred : 0) : -1; }
TArray<FString> I_GetSteamPath() { return {}; }
TArray<FString> I_GetGogPaths() { return {}; }
bool I_WriteIniFailed() { fprintf(stderr, "Unable to save CHEX config to SD\n"); return false; }
bool I_SetCursor(FTexture *) { return false; }
FStartupScreen *FStartupScreen::CreateInstance(int max_progress)
{ return new FStartupScreen(max_progress); }
void ST_Endoom() { I_Quit(); }
FString M_GetCachePath(bool create) { if (create) mkdir("/sdcard/doom/chex-cache",0777); return "/sdcard/doom/chex-cache/"; }
FString M_GetAutoexecPath() { return "/sdcard/doom/chex-autoexec.cfg"; }
FString M_GetCajunPath(const char *name)
{
    FString path = FString("/sdcard/doom/") + name;
    struct stat info;
    return stat(path.GetChars(), &info) == 0 && S_ISREG(info.st_mode)
        ? path : FString("");
}
FString M_GetConfigPath(bool) { return "/sdcard/doom/chex-zdoom.ini"; }
FString M_GetSavegamesPath() { return "/sdcard/doom/chex-saves/"; }
FString M_GetScreenshotsPath() { return "/sdcard/doom/"; }
bool I_InitNetwork()
{
    memset(&doomcom,0,sizeof(doomcom)); doomcom.ticdup = 1;
    doomcom.id = DOOMCOM_ID; doomcom.numplayers = doomcom.numnodes = 1;
    doomcom.consoleplayer = 0; netgame = multiplayer = false;
    return false;
}
void I_NetCmd() { doomcom.remotenode = -1; }
void I_SetMouseCapture() {}
void I_ReleaseMouseCapture() {}
FString I_GetFromClipboard(bool) { return ""; }
void I_PutInClipboard(const char *) {}
int I_PlayMovie(const char *) { return -1; }
