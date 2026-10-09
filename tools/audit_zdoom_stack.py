"""Audit compiler stack-usage reports for the RV32 core."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
rows=[]
for path in (root/'research/idf/build/esp-idf/zdoom_core').rglob('*.su'):
 for line in path.read_text().splitlines():
  fields=line.rsplit('\t',2)
  if len(fields)!=3:continue
  try:size=int(fields[1])
  except ValueError:continue
  rows.append({'function':fields[0].replace(str(root)+'/',''),'bytes':size,'kind':fields[2]})
renderer=[r for r in rows if '/r_segs.cpp:' in r['function']]
if not renderer:raise SystemExit('Missing r_segs.cpp compiler stack reports')
maximum=max(r['bytes'] for r in renderer)
if maximum>8192:raise SystemExit(f'Renderer scratch frame too large: {maximum}')
report={'reported_functions':len(rows),'renderer_max_frame_bytes':maximum,
 'largest_frames':sorted(rows,key=lambda r:r['bytes'],reverse=True)[:12],
 'note':'Individual frames only, not a call-chain bound. Screenshot/ancient ZIP routines still have >32 KiB frames; not exercised in E1M1 trial.'}
(root/'research/STACK_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Renderer stack regression: PASS; largest r_segs.cpp frame {maximum} bytes')
