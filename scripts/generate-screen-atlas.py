"""Generate code-authored display glyphs; UV tiles are four columns by two rows.

Top row: item input/output, fluid input/output. Bottom: hub, power, then two
repeats of the hub glyph that no model's UVs reference.
Keep the existing texture asset name to preserve saved/material references.
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]

def flow_arrow(draw, output, accent):
    """One filled outline: IN approaches the symbol above; OUT leaves it."""
    tail, tip = (340, 419) if output else (419, 340)
    direction = 1 if output else -1
    neck = tip - direction * 30
    draw.polygon([(249, tail), (249, neck), (226, neck), (256, tip),
                  (286, neck), (263, neck), (263, tail)], fill=accent)

def generate():
    tile=512
    atlas=Image.new('RGB',(tile*4,tile*2),(3,8,11))
    for index in range(8):
        canvas=Image.new('RGB',(tile,tile),(3,8,11))
        d=ImageDraw.Draw(canvas)
        white=(216,246,252)
        accent=(252,165,52) if index in (1,3,5) else (65,206,229)
        symbol_top=(255,193,99) if index in (1,3) else (119,222,247)
        symbol_dark=(175,87,20) if index in (1,3) else (34,126,171)
        symbol_light=(245,147,37) if index in (1,3) else (55,179,224)
        d.rounded_rectangle((22,22,490,490),radius=12,outline=(21,68,80),width=3)
        for x,y,sx,sy in ((32,32,1,1),(480,32,-1,1),(32,480,1,-1),(480,480,-1,-1)):
            d.line([(x+sx*36,y),(x,y),(x,y+sy*36)],fill=accent,width=5)
        if index in (0,1):
            # Isometric cargo cube with clean facets.
            d.polygon([(256,94),(348,146),(256,202),(164,146)],fill=symbol_top)
            d.polygon([(164,159),(249,211),(249,317),(164,265)],fill=symbol_dark)
            d.polygon([(263,211),(348,159),(348,265),(263,317)],fill=symbol_light)
            flow_arrow(d, index == 1, accent)
        elif index in (2,3):
            # Smooth droplet from a sampled teardrop outline.
            points=[(256,87),(184,188)]
            points.extend((256+85*math.cos(t),211+91*math.sin(t)) for t in [math.pi-i*math.pi/40 for i in range(41)])
            points.append((256,87))
            d.polygon(points,fill=symbol_light)
            d.arc((199,159,307,277),65,152,fill=symbol_top,width=9)
            flow_arrow(d, index == 3, accent)
        elif index in (4,6,7):
            nodes=[(256,125),(147,335),(365,335)]
            for point in nodes:
                d.line([(256,252),point],fill=accent,width=11)
            d.ellipse((218,214,294,290),fill=white)
            for x,y in nodes:
                d.ellipse((x-27,y-27,x+27,y+27),fill=(3,8,11),outline=white,width=9)
        else:
            d.polygon([(266,83),(176,274),(244,274),(222,416),(343,219),(271,219),(300,83)],fill=accent)
        # Quiet status strip, avoiding the old graph-paper/raised-bar look.
        d.line((160,451,352,451),fill=(18,47,56),width=3)
        d.ellipse((249,443,263,457),fill=accent)
        atlas.paste(canvas,((index%4)*tile,(index//4)*tile))
    path=ROOT/'assets/textures/T_TeleporterScreen_Grid.png'
    path.parent.mkdir(exist_ok=True,parents=True)
    atlas.save(path,optimize=True)
    print(path)

if __name__=='__main__':
    generate()
