#!/usr/bin/env python3
"""Inventory unresolved symbols of the experimental archive, not a linked game."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def symbols(output):
    return {line.split()[-1] for line in output.splitlines()
            if line.strip() and not line.endswith(':') and len(line.split()) >= 2}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=ROOT /
                        'research/idf/build/esp-idf/zdoom_core/libzdoom_core.a')
    parser.add_argument('--output', type=Path, default=ROOT /
                        'research/ARCHIVE_AUDIT.json')
    args = parser.parse_args()
    defined = symbols(subprocess.check_output(
        ['riscv32-esp-elf-nm', '-g', '--defined-only', str(args.archive)], text=True))
    undefined = symbols(subprocess.check_output(
        ['riscv32-esp-elf-nm', '-g', '-u', str(args.archive)], text=True))
    remaining = sorted(undefined - defined)
    decoded = subprocess.check_output(['riscv32-esp-elf-c++filt'],
                                     input='\n'.join(remaining), text=True).splitlines()
    hooks = [name for name in decoded if name.startswith('I_')]
    report = {'archive': str(args.archive.relative_to(ROOT)),
              'external_symbol_count': len(remaining),
              'platform_named_symbols': hooks,
              'external_symbols': decoded,
              'note': 'Símbolos externos incluem libc/libstdc++, bibliotecas de '
                      'compressão, áudio e plataforma. A lista não demonstra '
                      'link do engine completo nem requisitos finais de RAM.'}
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(f'{len(remaining)} external symbols; {len(hooks)} named I_*; '
          f'inventory: {args.output}')


if __name__ == '__main__':
    main()
