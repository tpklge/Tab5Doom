"""Check retained registration tables in the linked RV32 executable."""
from pathlib import Path
import json
import shutil
import subprocess
root=Path(__file__).resolve().parents[1]
compiler=Path.home()/'.espressif/tools/riscv32-esp-elf/esp-14.2.0_20260121/riscv32-esp-elf/bin'
nm=shutil.which('riscv32-esp-elf-nm') or str(compiler/'riscv32-esp-elf-nm')
elf=root/'research/idf/build/Tab5CHEX_ZDoomExperimental.elf'
lines=subprocess.check_output([nm,'-S','-C',str(elf)],text=True).splitlines()
symbols={}
for line in lines:
 parts=line.split(maxsplit=3)
 if len(parts)==4:
  try:symbols[parts[3]]=(int(parts[0],16),int(parts[1],16),parts[2])
  except ValueError:pass
report={}
for kind in 'ACGMY':
 head=symbols[kind+'RegHead'][0];tail=symbols[kind+'RegTail'][0]
 assert head<tail and (tail-head)%4==0,(kind,head,tail)
 count=(tail-head)//4-1
 assert count>0,(kind,count)
 assert 0x4ff00000<=head<0x50000000,(kind,'table is not writable SRAM')
 report[kind]={'head':hex(head),'tail':hex(tail),'pointer_slots':count}
classes=[v[0] for k,v in symbols.items() if k.endswith('::RegistrationInfoPtr')]
assert len(classes)>100,len(classes)
assert all(symbols['CRegHead'][0]<address<symbols['CRegTail'][0] for address in classes)
assert 'APlayerPawn::RegistrationInfoPtr' in symbols
assert 'Tab5FrameBuffer::RegistrationInfoPtr' in symbols
(root/'research/REGISTRY_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n')
print('RV32 registry audit: PASS', {k:v['pointer_slots'] for k,v in report.items()})
