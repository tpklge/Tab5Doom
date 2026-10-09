#!/usr/bin/env python3
"""Regression for the legacy UDMF predicate exposed by the modern compiler."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'research/zdoom-2.8.1/src'
source = (ENGINE / 'p_udmf.cpp').read_text()
start = source.index('static inline bool P_IsThingSpecial(')
end = source.index('\n}', start) + 2
function = source[start:end]
actors = (ENGINE / 'thingdef/thingdef_codeptr.cpp').read_text()
flags_start = actors.index('enum KILS\n')
flags_end = actors.index('\n};', flags_start) + 3
kill_start = actors.index('static void DoKill(')
damage_start = actors.index('int dmgFlags = ', kill_start)
damage_end = actors.index('if ((killtarget->flags', damage_start)
damage = actors[damage_start:damage_end]
harness = '''
#include <cassert>
#define DEFINE_SPECIAL(name, number, ...) name = number,
enum Special {
#include "actionspecials.h"
};
'''+function+actors[flags_start:flags_end]+'''
enum { DMG_NO_ARMOR=1, DMG_NO_FACTOR=16, DMG_FOILINVUL=64, DMG_FOILBUDDHA=128 };
int damage_flags(int flags) {
'''+damage+'''return dmgFlags;
}
int main() {
    for (int value = -1; value <= 300; value++) {
        bool expected = (value >= 134 && value <= 137)
                        || value == 139 || value == 175 || value == 178;
        assert(P_IsThingSpecial(value) == expected);
    }
    assert(damage_flags(0) == 17);
    assert(damage_flags(KILS_FOILINVUL) == 81);
    assert(damage_flags(KILS_FOILBUDDHA) == 145);
    assert(damage_flags(KILS_FOILINVUL | KILS_FOILBUDDHA) == 209);
    assert(damage_flags(KILS_KILLMISSILES) == 17);
}
'''
with tempfile.TemporaryDirectory(prefix='chex-specials-') as tmp:
    root = Path(tmp)
    code = root / 'specials.cpp'
    binary = root / 'specials'
    code.write_text(harness)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall',
                    '-Wextra', '-Werror', '-I', str(ENGINE), str(code),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: UDMF classification across 302 values and five damage-flag cases')
