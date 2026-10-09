"""Exercise actual allocator wrappers with ESP-style NULL-on-zero behavior."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'research/zdoom-2.8.1/src/m_alloc.cpp').read_text()
def extract(signature):
 start=source.index(signature);opening=source.index('{',start);depth=1;end=opening+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
prefix=r'''
#include <stdlib.h>
#include <malloc/malloc.h>
#include <assert.h>
namespace GC { size_t AllocBytes=0; }
void I_FatalError(const char *,...) { abort(); }
static void *esp_malloc(size_t size) { return size ? malloc(size) : nullptr; }
static void *esp_realloc(void *p,size_t size) { if(!size) { free(p);return nullptr; } return realloc(p,size); }
#define malloc esp_malloc
#define realloc esp_realloc
#define _msize malloc_size
#define ESP_PLATFORM 1
'''
harness=r'''
int main() {
 void *p=M_Malloc(0);assert(p);assert(GC::AllocBytes==malloc_size(p));
 p=M_Realloc(p,64);assert(p);assert(GC::AllocBytes==malloc_size(p));
 p=M_Realloc(p,0);assert(p);assert(GC::AllocBytes==malloc_size(p));free(p);
 GC::AllocBytes=0;p=M_Realloc(nullptr,0);assert(p);assert(GC::AllocBytes==malloc_size(p));free(p);
}
'''
with tempfile.TemporaryDirectory() as folder:
 p=Path(folder);(p/'test.cpp').write_text(prefix+extract('void *M_Malloc(size_t size)')+'\n'+extract('void *M_Realloc(void *memblock, size_t size)')+harness)
 subprocess.run(['c++','-std=c++11','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('Zero-size allocator regression: PASS (ASan/UBSan)')
