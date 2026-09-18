"""Shared, non-emissive RGBA markings for all TeleportLogistics models."""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
image=Image.new('RGBA',(2048,1024))
draw=ImageDraw.Draw(image)
# Two lines per plate: the kind on the first, the rest on the second, so the type
# can be set large enough to read in game beside a mark at nameplate scale.
plates=(('ITEM','TELEPORTER (IN)'),('ITEM','TELEPORTER (OUT)'),
        ('FLUID','TELEPORTER (IN)'),('FLUID','TELEPORTER (OUT)'),
        ('TELEPORTER','HUB'),('PERSONNEL','TELEPORTER'))
# Each plate occupies a 512 px tile, but the quad only samples a band of it. The
# plate is 81x18 model units, about 4.5:1, so a band 512x114 keeps the art square;
# the old 512x200 band squashed it horizontally by 1.76. Mark and lettering sit on
# one line across the full width instead of stacking with dead space either side.
TILE,MARGIN=512,20
BAND_TOP,BAND_HEIGHT=176,114
BAND_MID=BAND_TOP+BAND_HEIGHT//2
MARK_W=206                  # FICSIT mark is 4:1, so 206 wide reads 51 tall
TEXT_X=MARK_W+52            # lettering starts clear of the mark
TEXT_W=TILE-TEXT_X-14
face=str(root/'assets/vendor/OpenSans/OpenSans-Bold.ttf')
def widest(size):
 f=ImageFont.truetype(face,size)
 return max(f.getbbox(t)[2]-f.getbbox(t)[0] for plate in plates for t in plate)
size=next(s for s in range(46,11,-1) if widest(s)<=TEXT_W)
font=ImageFont.truetype(face,size)
LINE=int(size*1.16)
assert widest(size)<=TEXT_W,'plate lettering does not fit beside the mark'
assert 2*LINE<=BAND_HEIGHT,'two lines of lettering do not fit the plate band'

def letter(x,y,lines):
 for i,text in enumerate(lines):
  draw.text((x+TEXT_X,y+BAND_MID+(i-0.5)*LINE),text,font=font,fill=(25,29,30,255),anchor='lm')
outline=json.loads((root/'assets/vendor/FICSIT/Ficsit-outline.json').read_text())
# The outline is authored Y-up and Pillow draws Y-down, so py is negated; drawn
# as-is the wordmark is vertically mirrored. Checked against Ficsit-logo-reference.png.
for i,label in enumerate(plates[:5]):
 x,y=(i%4)*512,(i//4)*512
 # All artwork sits inside a generous transparent margin for mip safety.
 for poly in outline['polygons']:
  draw.polygon([(x+MARGIN+MARK_W/2+px*MARK_W,y+BAND_MID-py*MARK_W) for px,py in poly],fill=(25,29,30,255))
 letter(x,y,label)
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
 draw.polygon([(1536+MARGIN+MARK_W/2+px*MARK_W,512+BAND_MID-py*MARK_W) for px,py in poly],fill=(25,29,30,255))
letter(1536,512,plates[5])
p=root/'assets/textures/T_TeleporterDetails.png';image.save(p)
print(f'{p} (lettering at {size} px, widest plate {widest(size)} px in a {TILE} px tile)')
