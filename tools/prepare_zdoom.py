#!/usr/bin/env python3
"""Generate native-parser headers and engine resources before the IDF build."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'research/zdoom-2.8.1'
EXPECTED_COMMIT = '1a9bc53d84b5cceb567fd8246c44984aac88388a'


def run(*command, cwd=None):
    subprocess.run(list(map(str, command)), cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build', type=Path,
                        default=ROOT / 'research/host-tools/build')
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'research/generated')
    args = parser.parse_args()
    commit = subprocess.check_output(['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'],
                                     text=True).strip()
    if commit != EXPECTED_COMMIT:
        raise SystemExit('A árvore ZDoom não está no commit esperado')
    native = args.native_build.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    run('cmake', '-S', ROOT / 'research/host-tools', '-B', native,
        '-DCMAKE_BUILD_TYPE=Release')
    run('cmake', '--build', native, '--parallel', '4')
    grammar = output / 'xlat_parser.y'
    shutil.copy2(SOURCE / 'src/xlat/xlat_parser.y', grammar)
    run(native / 'lemon/lemon', grammar.name, cwd=output)
    run(native / 're2c/re2c', '--no-generation-date', '-s', '-o',
        output / 'sc_man_scanner.h', SOURCE / 'src/sc_man_scanner.re')
    revision = subprocess.check_output(['git', '-C', str(SOURCE), 'show', '-s',
                                        '--format=%cI', 'HEAD'], text=True).strip()
    (output / 'gitinfo.h').write_text(
        f'#define GIT_DESCRIPTION "2.8.1-tab5-experimental"\n'
        f'#define GIT_HASH "{commit}"\n#define GIT_TIME "{revision}"\n')
    # Target RV32 is little-endian with IEEE binary64 double and 32-bit pointers.
    (output / 'gd_qnan.h').write_text('#define f_QNAN 0x7fc00000\n#define d_QNAN0 0\n#define d_QNAN1 0x7ff80000\n')
    # Never run host arithchk and reuse its LP64 result for this target.
    (output / 'arith.h').write_text(
        '#define IEEE_8087\n#define Arith_Kind_ASL 1\n'
        '#define Double_Align\n')
    # ZIP_DEFLATED is readable by ZDoom and avoids host LZMA/bzip dependencies.
    # Fixed dates and sorted paths make the resource archive reproducible.
    package = output / 'zdoom.pk3'
    static = SOURCE / 'wadsrc/static'
    count = 0
    with zipfile.ZipFile(package, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(static.rglob('*')):
            if not file.is_file():
                continue
            entry = zipfile.ZipInfo(file.relative_to(static).as_posix(),
                                    date_time=(1980, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, file.read_bytes())
            count += 1
    for name in ('xlat_parser.c', 'xlat_parser.h', 'sc_man_scanner.h'):
        if not (output / name).stat().st_size:
            raise SystemExit(f'Arquivo gerado vazio: {name}')
    with zipfile.ZipFile(package) as archive:
        if archive.testzip() is not None:
            raise SystemExit('Falha na integridade do PK3')
        if 'iwadinfo.txt' not in archive.namelist():
            raise SystemExit('PK3 sem identificação dos jogos')
    metadata = {'engine_commit': commit, 'package_files': count,
                'package_bytes': package.stat().st_size,
                'package_sha256': hashlib.sha256(package.read_bytes()).hexdigest()}
    (output / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
