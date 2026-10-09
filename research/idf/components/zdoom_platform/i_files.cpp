#include "i_system.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include <string>

struct Search {
    findstate_t *state;
    std::string directory;
    std::vector<dirent *> names;
    ~Search() { for (auto name : names) free(name); }
};
static char lower(char c) { return c >= 'A' && c <= 'Z' ? c + ('a'-'A') : c; }
static bool matches(const char *pattern, const char *name)
{
    const char *star = nullptr, *retry = nullptr;
    while (*name) {
        if (*pattern == '?' || lower(*pattern) == lower(*name)) { ++pattern; ++name; }
        else if (*pattern == '*') { star = pattern++; retry = name; }
        else if (star) { pattern = star + 1; name = ++retry; }
        else return false;
    }
    while (*pattern == '*') ++pattern;
    return !*pattern;
}
void *I_FindFirst(const char *spec, findstate_t *state)
{
    state->count = state->current = 0; state->namelist = nullptr;
    state->platform_context = nullptr;
    std::string path(spec); auto slash = path.find_last_of('/');
    std::string directory = slash == std::string::npos ? "." : path.substr(0, slash + 1);
    std::string pattern = slash == std::string::npos ? path : path.substr(slash + 1);
    DIR *dir = opendir(directory.c_str());
    if (!dir) return reinterpret_cast<void *>(-1);
    Search *search = new Search; search->state = state; search->directory = directory;
    while (dirent *entry = readdir(dir)) {
        if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) continue;
        if (!matches(pattern.c_str(), entry->d_name)) continue;
        dirent *copy = static_cast<dirent *>(calloc(1, sizeof(dirent)));
        if (!copy) { closedir(dir); delete search; I_FatalError("Out of memory enumerating %s", spec); }
        strncpy(copy->d_name, entry->d_name, sizeof(copy->d_name)-1); search->names.push_back(copy);
    }
    closedir(dir);
    if (search->names.empty()) { delete search; return reinterpret_cast<void *>(-1); }
    state->count = search->names.size(); state->namelist = search->names.data();
    state->platform_context = search; return search;
}
int I_FindNext(void *handle, findstate_t *state)
{
    if (!handle || handle == reinterpret_cast<void *>(-1)) return -1;
    return ++state->current < state->count ? 0 : -1;
}
int I_FindClose(void *handle)
{
    if (!handle || handle == reinterpret_cast<void *>(-1)) return -1;
    Search *search = static_cast<Search *>(handle);
    search->state->platform_context = nullptr; search->state->namelist = nullptr;
    search->state->count = 0; delete search; return 0;
}
int I_FindAttr(findstate_t *state)
{
    if (!state->platform_context || state->current >= state->count) return 0;
    auto search = static_cast<Search *>(state->platform_context);
    std::string path = search->directory;
    if (!path.empty() && path.back() != '/') path += '/';
    path += I_FindName(state); struct stat info;
    int attributes = I_FindName(state)[0] == '.' ? FA_HIDDEN : 0;
    if (stat(path.c_str(), &info) == 0) {
        if (S_ISDIR(info.st_mode)) attributes |= FA_DIREC;
        if (!(info.st_mode & S_IWUSR)) attributes |= FA_RDONLY;
    }
    return attributes;
}
