"""Compile the actual ESP directory backend against a minimal host contract."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
header='''#pragma once
#include <dirent.h>
struct findstate_t { int count; dirent **namelist; int current; void *platform_context; };
void *I_FindFirst(const char *,findstate_t *);
int I_FindNext(void *,findstate_t *);
int I_FindClose(void *);
int I_FindAttr(findstate_t *);
void I_FatalError(const char *,...);
#define I_FindName(s) ((s)->namelist[(s)->current]->d_name)
#define FA_RDONLY 1
#define FA_HIDDEN 2
#define FA_DIREC 8
'''
harness=r'''
#include "i_system.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <set>
void I_FatalError(const char *,...) { abort(); }
static void file(const std::string &path) { FILE *f=fopen(path.c_str(),"wb");assert(f);fclose(f); }
int main(int argc,char **argv) {
 assert(argc==2);std::string base=argv[1];
 file(base+"/CHEX3.WAD");file(base+"/chex.wad");file(base+"/.hidden.wad");file(base+"/notes.txt");
 assert(!mkdir((base+"/nested.wad").c_str(),0700));
 char before[4096],after[4096];assert(getcwd(before,sizeof(before)));
 findstate_t state={};void *handle=I_FindFirst((base+"/*.WaD").c_str(),&state);
 assert(handle!=(void*)-1);std::set<std::string> names;
 do { names.insert(I_FindName(&state));int attr=I_FindAttr(&state);
      if(std::string(I_FindName(&state))=="nested.wad")assert(attr&FA_DIREC);
      if(std::string(I_FindName(&state))==".hidden.wad")assert(attr&FA_HIDDEN);
 } while(!I_FindNext(handle,&state));
 assert(names.size()==4);assert(names.count("CHEX3.WAD"));
 assert(!I_FindClose(handle));assert(!state.namelist&&!state.platform_context);
 handle=I_FindFirst((base+"/chex?.wad").c_str(),&state);
 assert(handle!=(void*)-1);assert(state.count==1);assert(std::string(I_FindName(&state))=="CHEX3.WAD");
 I_FindClose(handle);
 assert(I_FindFirst((base+"/absent/*.wad").c_str(),&state)==(void*)-1);
 assert(I_FindFirst((base+"/none*.wad").c_str(),&state)==(void*)-1);
 assert(getcwd(after,sizeof(after)));assert(std::string(before)==after);
}
'''
with tempfile.TemporaryDirectory() as folder:
 p=Path(folder);(p/'i_system.h').write_text(header);(p/'test.cpp').write_text(harness)
 data=p/'data';data.mkdir()
 subprocess.run(['c++','-std=c++11','-fsanitize=address,undefined','-I',str(p),
 str(root/'research/idf/components/zdoom_platform/i_files.cpp'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test'),str(data)],check=True)
print('ESP file backend regression: PASS (ASan/UBSan)')
