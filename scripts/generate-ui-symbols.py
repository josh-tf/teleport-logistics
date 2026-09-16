"""Draw milestone tile symbols without modifying building reward captures."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets/icons'
WHITE = (242, 245, 247, 255)
def symbol(name):
    im = Image.new('RGBA', (1024,1024)); d = ImageDraw.Draw(im)
    def line(points,w=36): d.line(points,fill=WHITE,width=w,joint='curve')
    if name=='Hub':
        for p in [(512,225),(257,720),(767,720)]:line([(512,492),p],42)
        for x,y,r in [(512,492,104),(512,225,73),(257,720,73),(767,720,73)]:
            d.ellipse((x-r,y-r,x+r,y+r),fill=WHITE)
            d.ellipse((x-r+25,y-r+25,x+r-25,y+r-25),fill=(0,0,0,0))
    else:
        d.arc((230,120,794,700),180,360,fill=WHITE,width=42)
        line([(230,410),(230,850),(365,850)]);line([(794,410),(794,850),(659,850)])
        d.ellipse((460,296,564,400),fill=WHITE)
        line([(512,435),(512,625)],52);line([(407,540),(512,465),(617,540)],38)
        line([(512,615),(445,792)],38);line([(512,615),(579,792)],38)
    return im
for name, asset in [('Hub', 'Milestone'), ('TravelHub', 'PersonnelMilestone')]:
    for size in (256, 512):
        symbol(name).resize((size, size), Image.Resampling.LANCZOS).save(OUT / f'T_Teleporter{asset}_{size}.png')
print('Generated Logistics and Personnel milestone symbols; building captures preserved')
