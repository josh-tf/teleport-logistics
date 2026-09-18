"""Shared, non-emissive RGBA markings for all TeleportLogistics models."""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
image=Image.new('RGBA',(2048,1024))
draw=ImageDraw.Draw(image)
plates=('ITEM TELEPORTER (In)','ITEM TELEPORTER (Out)','FLUID TELEPORTER (In)','FLUID TELEPORTER (Out)','TELEPORTER HUB','PERSONNEL TELEPORTER')
# Each plate occupies a 512 px tile. Lettering wider than the tile bleeds into the
# neighbouring plate's UV rectangle, so pick the largest size that clears a margin
# on both sides rather than trusting a hardcoded size to keep fitting.
TILE,MARGIN=512,20
face=str(root/'assets/vendor/OpenSans/OpenSans-Bold.ttf')
def widest(size):
 f=ImageFont.truetype(face,size)
 return max(f.getbbox(t)[2]-f.getbbox(t)[0] for t in plates)
size=next(s for s in range(38,11,-1) if widest(s)<=TILE-2*MARGIN)
font=ImageFont.truetype(face,size)
assert widest(size)<=TILE-2*MARGIN,'plate lettering does not fit its tile'
outline=json.loads((root/'assets/vendor/FICSIT/Ficsit-outline.json').read_text())
# The outline is authored Y-up and Pillow draws Y-down, so py is negated; drawn
# as-is the wordmark is vertically mirrored. Checked against Ficsit-logo-reference.png.
for i,label in enumerate(plates[:5]):
 x,y=(i%4)*512,(i//4)*512
 # All artwork sits inside a generous transparent margin for mip safety.
 for poly in outline['polygons']:
  draw.polygon([(x+176+px*160,y+164-py*160) for px,py in poly],fill=(25,29,30,255))
 draw.text((x+256,y+292),label,font=font,fill=(25,29,30,255),anchor='mm')
# Shared hazard stripe tile, cropped by its own authored UV rectangle.
hazard=Image.new('RGBA',(512,512));h=ImageDraw.Draw(hazard)
for i in range(-3,15):
 x=i*48
 h.polygon([(x,88),(x+24,88),(x-48,188),(x-72,188)],fill=(222,167,41,255))
image.paste(hazard,(512,512))
# Flat screw head tile: silhouette-changing fasteners remain geometry.
draw.ellipse((1104,592,1200,688),fill=(91,98,101,255),outline=(26,30,32,255),width=8)
draw.line((1124,668,1180,612),fill=(27,29,30,255),width=10)
for poly in outline['polygons']:
 draw.polygon([(1536+176+px*160,512+164-py*160) for px,py in poly],fill=(25,29,30,255))
draw.text((1792,804),plates[5],font=font,fill=(25,29,30,255),anchor='mm')
p=root/'assets/textures/T_TeleporterDetails.png';image.save(p)
print(f'{p} (lettering at {size} px, widest plate {widest(size)} px in a {TILE} px tile)')
