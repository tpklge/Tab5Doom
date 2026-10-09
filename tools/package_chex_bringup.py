"""Prepare the locally built firmware and SD files for a device trial."""
from pathlib import Path
import hashlib
import json
import shutil
root=Path(__file__).resolve().parents[1]
output=root/'build/chex3-bringup'
sd=output/'microSD/doom';sd.mkdir(parents=True,exist_ok=True)
files={
 root/'research/idf/build/Tab5CHEX_ZDoomExperimental.bin':output/'Tab5CHEX_Chex3_Experimental.bin',
 root/'research/generated/zdoom.pk3':sd/'zdoom.pk3',
 root/'research/assets/chex3.wad':sd/'chex3.wad',
 root/'research/assets/chex.wad':sd/'chex.wad',
 root/'research/assets/CQ3 ReadMe.txt':sd/'CQ3 ReadMe.txt',
 root/'research/assets/version1_4.txt':sd/'version1_4.txt',
}
if (root/'research/assets/chex2.wad').is_file():
 files[root/'research/assets/chex2.wad']=sd/'chex2.wad'
manifest={'engine':'ZDoom 2.8.1','engine_commit':'1a9bc53d84b5cceb567fd8246c44984aac88388a',
 'game':'Chex Quest 1/2 originals and Chex Quest 3 v1.4','target':'ESP32-P4 / M5Stack Tab5',
 'startup':'three-game selector','startup_map':'E1M1','audio':True,'music':False,'sfx_voices':16,'sample_rate':22050,'device_validated':False,'files':[]}
for source,destination in files.items():
 if not source.is_file():raise SystemExit(f'Missing build/input: {source}')
 shutil.copy2(source,destination)
 manifest['files'].append({'path':str(destination.relative_to(output)),
 'bytes':destination.stat().st_size,'sha256':hashlib.sha256(destination.read_bytes()).hexdigest()})
(output/'MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(output)
