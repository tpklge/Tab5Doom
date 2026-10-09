#!/usr/bin/env python3
"""Run the patched directory loader against real files without changing cwd."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'research/zdoom-2.8.1/src/d_main.cpp').read_text()
start = source.index('static void D_AddDirectory (')
opening = source.index('{', start)
end = opening + 1
depth = 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
function = source[start:end]

harness = r'''
#include <cassert>
#include <cstring>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <set>
#include <string>
#include <vector>
#define ESP_PLATFORM 1
#define stricmp strcasecmp
struct FString : std::string {
    using std::string::string;
    size_t Len() const { return size(); }
    const char *GetChars() const { return c_str(); }
};
template<typename T> using TArray = std::vector<T>;
static void D_AddFile(TArray<FString> &files, const char *path) {
    files.emplace_back(path);
}
'''
harness += function
harness += r'''
int main(int argc, char **argv) {
    assert(argc == 2);
    char before[4096], after[4096];
    assert(getcwd(before, sizeof(before)));
    TArray<FString> files;
    D_AddDirectory(files, argv[1]);
    assert(files.size() == 2);
    std::set<std::string> found(files.begin(), files.end());
    assert(found.count(std::string(argv[1]) + "/chex.wad"));
    assert(found.count(std::string(argv[1]) + "/CHEX3.WAD"));
    files.clear();
    std::string with_slash = std::string(argv[1]) + "/";
    D_AddDirectory(files, with_slash.c_str());
    assert(files.size() == 2);
    assert(std::set<std::string>(files.begin(), files.end()) == found);
    D_AddDirectory(files, "/nonexistent-tab5chex-path");
    assert(files.size() == 2);
    assert(getcwd(after, sizeof(after)));
    assert(strcmp(before, after) == 0);
}
'''
with tempfile.TemporaryDirectory(prefix='chex-paths-') as tmp:
    root = Path(tmp)
    directory = root / 'doom'
    directory.mkdir()
    for name in ('chex.wad', 'CHEX3.WAD', '.hidden.wad', 'README.txt'):
        (directory / name).write_bytes(b'test')
    (directory / 'subdir.wad').mkdir()
    (directory / 'subdir.wad/nested.wad').write_bytes(b'test')
    code = root / 'paths.cpp'
    binary = root / 'paths'
    code.write_text(harness)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall',
                    '-Wextra', '-Werror', str(code), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(directory)], check=True)
print('PASS: absolute paths, uppercase, directory filtering and unchanged cwd')
