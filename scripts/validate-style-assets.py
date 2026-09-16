"""Check authored LOD integrity, detail removal and hollow-port collision."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
for name in ('ItemInput','ItemOutput','FluidInput','FluidOutput','Hub','TravelHub'):
    base=ROOT/'assets/models'/f'SM_Teleporter{name}.obj'
    counts=[]
    for level in (0,1,2):
        path=base if not level else base.parent/'lods'/f'{base.stem}_LOD{level}.obj'
        current=''; count=0
        for line in path.read_text().splitlines():
            if line.startswith('usemtl '):current=line[7:]
            if line.startswith('f '):
                count+=len(line.split())-3
                assert not level or not current.startswith('decal_'), (path,current)
        counts.append(count)
    assert counts[0]>counts[1]>counts[2]>1000,(name,counts)
    print(name,counts)
shapes=json.loads((ROOT/'assets/models/collision.json').read_text())
for name in ('ItemInput','ItemOutput','FluidInput','FluidOutput'):
    z=100 if name.startswith('Item') else 175
    for x in (160,180,200,220):
        for y in (-25,0,25):
            assert not any(all(abs(p-c)<s/2 for p,c,s in zip((x,y,z),b['center'],b['size'])) for b in shapes[name]), (name,x,y,z)
print('PASS: three descending LODs, no distant decals, and connector paths clear of simple collision')
