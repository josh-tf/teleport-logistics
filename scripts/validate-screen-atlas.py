"""Check exported display faces select the correct network-direction icon."""
from pathlib import Path
import math
from PIL import Image
root=Path(__file__).resolve().parents[1]
assert Image.open(root/'assets/textures/T_TeleporterScreen_Grid.png').size==(2048,1024)
for name,expected in (('ItemInput',{0}),('ItemOutput',{1}),('FluidInput',{2}),('FluidOutput',{3}),('Hub',{4,5}),('TravelHub',{4})):
    uv=[]; material=None; used=set(); faces=0
    for line in (root/f'assets/models/SM_Teleporter{name}.obj').read_text().splitlines():
        if line.startswith('vt '): uv.append(tuple(map(float,line.split()[1:3])))
        elif line.startswith('usemtl '): material=line.split()[1]
        elif line.startswith('f ') and material=='screen':
            corners=[uv[int(token.split('/')[1])-1] for token in line.split()[1:]]
            tiles=set()
            for u,v in corners:
                assert 0<u<1 and 0<v<1,(name,u,v)
                col=math.floor(u*4); row=math.floor((1-v)*2)
                tiles.add(row*4+col)
                assert .01 < u*4-col < .99 and .01 < (1-v)*2-row < .99,(name,'tile edge bleed')
            assert len(tiles)==1,(name,'display face crosses atlas tiles')
            used.update(tiles);faces+=1
    assert used==expected and faces>=2,(name,used,expected,faces)
print('PASS: all six models select their correct flush-display atlas tiles without edge bleed')
