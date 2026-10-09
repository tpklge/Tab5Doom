"""Build real bundled libraries and run the shared decoding regression."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
source = root / 'research/probe/compression-native'
build = root / 'research/generated/compression-native'
subprocess.run(['cmake', '-S', str(source), '-B', str(build)], check=True)
subprocess.run(['cmake', '--build', str(build), '-j', '4'], check=True)
subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure'], check=True)
