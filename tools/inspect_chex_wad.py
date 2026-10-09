#!/usr/bin/env python3
"""Inspect WAD structure and engine requirements without assuming IWAD magic."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

MAP = re.compile(r'(?:E\dM\d|MAP\d\d)\Z')
SCRIPT_LUMPS = ('DECORATE', 'MAPINFO', 'SNDINFO', 'ANIMDEFS', 'BEHAVIOR',
                'ZSCRIPT', 'LOADACS')


def inspect(path):
    size = path.stat().st_size
    with path.open('rb') as stream:
        header = stream.read(12)
        if len(header) != 12:
            raise ValueError('cabeçalho truncado')
        magic, count, offset = struct.unpack('<4sii', header)
        if magic not in (b'IWAD', b'PWAD'):
            raise ValueError('assinatura diferente de IWAD/PWAD')
        if count <= 0 or offset < 12 or offset + count * 16 > size:
            raise ValueError('diretório de lumps fora do arquivo')
        stream.seek(offset)
        names = []
        for index in range(count):
            pos, length, label = struct.unpack('<ii8s', stream.read(16))
            if pos < 0 or length < 0 or pos + length > size:
                raise ValueError(f'lump {index} fora do arquivo')
            names.append(label.rstrip(b'\0').decode('ascii').upper())
        stream.seek(0)
        digest = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    maps = []
    for index, name in enumerate(names):
        if not MAP.fullmatch(name):
            continue
        end = next((j for j in range(index + 1, len(names))
                    if MAP.fullmatch(names[j])), len(names))
        maps.append({'name': name, 'behavior': 'BEHAVIOR' in names[index + 1:end]})
    requirements = [name for name in SCRIPT_LUMPS if name in names]
    return {'file': str(path), 'bytes': size, 'signature': magic.decode(),
            'sha256': digest.hexdigest(), 'lumps': count, 'map_markers': maps,
            'script_lumps': requirements, 'dehacked': 'DEHACKED' in names,
            'requires_extended_engine': bool(requirements),
            'note': 'Cabeçalho PWAD não implica arquivo inválido; compatibilidade '
                    'não é demonstrada apenas por esta inspeção.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('wads', nargs='+', type=Path)
    args = parser.parse_args()
    reports = []
    failed = False
    for path in args.wads:
        try:
            reports.append(inspect(path))
        except (OSError, ValueError, UnicodeError, struct.error) as error:
            reports.append({'file': str(path), 'error': str(error)})
            failed = True
    print(json.dumps(reports, indent=2, ensure_ascii=False))
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
