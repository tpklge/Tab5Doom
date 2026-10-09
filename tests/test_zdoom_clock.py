"""Exercise the actual portable clock used by the IDF timing backend."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
header = root / 'research/idf/components/zdoom_platform/tic_clock.h'
source = r'''
#include "tic_clock.h"
#include <assert.h>
int main() {
    ChexTicClock clock;
    clock.reset(1000000);
    assert(clock.ticks(1000000, true) == 0);
    assert(clock.fraction(1000000, nullptr) == 0);
    assert(clock.ticks(1999999, false) == 34);
    assert(clock.ticks(2000000, false) == 35);
    clock.freeze(1000100, true); // Freezing at tic zero must work.
    assert(clock.ticks(9000000, false) == 0);
    assert(clock.realtime(9000000) == 8000000);
    clock.freeze(9000000, true); // Repeated freeze preserves the origin.
    clock.freeze(9000000, false);
    assert(clock.ticks(9000000, false) == 0);
    assert(clock.ticks(9999900, false) == 35);
    clock.reset(0);
    assert(clock.fraction(0, nullptr) == 65536);
    clock.ticks(0, true); // Timestamp zero is a valid saved sample.
    uint32_t deadline;
    assert(clock.fraction(10000, &deadline) == 22937);
    assert(deadline == 28);
    assert(clock.fraction(1000000, nullptr) == 65536);
    assert(clock.ticks(86400000000ULL, false) == 3024000);
}
'''
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder)
    (path / 'clock.cpp').write_text(source)
    subprocess.run(['c++', '-std=c++11', '-I', str(header.parent),
                    str(path / 'clock.cpp'), '-o', str(path / 'clock')], check=True)
    subprocess.run([str(path / 'clock')], check=True)
print('Clock regression: PASS')
