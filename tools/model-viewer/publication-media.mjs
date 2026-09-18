// Compose listing artwork from the shipped model captures, without external requests.
import {chromium} from 'playwright';
import {readFile,mkdir,copyFile} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const out=path.join(root,'docs/publishing/media');
await mkdir(out,{recursive:true});
const models=[['ItemInput','Item Input','TIER 5'],['ItemOutput','Item Output','TIER 5'],['FluidInput','Fluid Input','TIER 5'],['FluidOutput','Fluid Output','TIER 5'],['Hub','Network Hub','TIER 5'],['TravelHub','Personnel','TIER 9']];
const cards=[];
for(const [name,label,tier] of models){
 const source=path.join(root,`assets/icons/T_Teleporter${name}_512.png`);
 await copyFile(source,path.join(out,`${name}.png`));
 const data=(await readFile(source)).toString('base64');
 cards.push(`<article${tier==='TIER 9'?' class="late"':''}><img alt="${label}" src="data:image/png;base64,${data}"><p>${label}</p><span>${tier}</span></article>`);
}
// Open Sans already ships with the mod for the in-world plates. Embedding it keeps
// the banner off whatever the headless browser would otherwise fall back to.
const face=(await readFile(path.join(root,'assets/vendor/OpenSans/OpenSans-Bold.ttf'))).toString('base64');
await copyFile(path.join(root,'assets/icons/T_TeleporterMilestone_512.png'),path.join(out,'icon-512.png'));
const html=`<!doctype html><html lang="en"><meta charset="utf-8"><title>Teleport Logistics model preview</title><style>
@font-face{font-family:Plate;src:url(data:font/ttf;base64,${face}) format('truetype');font-weight:700}
*{box-sizing:border-box}
body{margin:0;width:1600px;height:900px;color:#eef1f3;font-family:Arial,Helvetica,sans-serif;
 background:#141a1f radial-gradient(1200px 620px at 22% 4%,#20282f 0%,#141a1f 62%);
 padding:60px 72px;display:flex;flex-direction:column}
header{display:flex;align-items:flex-start;gap:26px}
.rail{width:7px;align-self:stretch;background:linear-gradient(#f0a340,#c9761d);flex:none}
h1{font:700 78px/1 Plate,Arial,sans-serif;letter-spacing:.055em;margin:0}
h2{font-size:25px;font-weight:400;color:#aeb9c2;margin:14px 0 0;letter-spacing:.01em}
section{display:grid;grid-template-columns:repeat(6,1fr);gap:16px;flex:1;margin:44px 0 0;min-height:0}
article{background:linear-gradient(#212a31,#1a2127);border:1px solid #2c363f;border-top:3px solid #ee9e43;
 border-radius:5px;padding:16px 10px 18px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:4px}
article.late{border-top-color:#41cebe}
img{width:100%;height:auto;object-fit:contain;flex:1;min-height:0}
p{font:700 21px/1.2 Plate,Arial,sans-serif;margin:6px 0 2px;text-align:center;letter-spacing:.01em}
span{font-size:12px;letter-spacing:.20em;color:#8d99a3}
article.late span{color:#41cebe}
footer{display:flex;justify-content:space-between;align-items:center;border-top:1px solid #2c363f;
 margin-top:34px;padding-top:22px;color:#98a4ad;font-size:20px}
b{color:#ee9e43;font-weight:400}
</style><header><div class="rail"></div><div><h1>TELEPORT LOGISTICS</h1>
<h2>Named routes for your factory. Personal travel for your Pioneer.</h2></div></header>
<section>${cards.join('')}</section>
<footer><span><b>TIER 5</b> Teleport Logistics &nbsp;&nbsp;·&nbsp;&nbsp; <b>TIER 9</b> Teleport Personnel Transport</span><span>Unofficial Satisfactory mod · Model preview</span></footer></html>`;
const browser=await chromium.launch({headless:true,executablePath:process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE || chromium.executablePath(),args:['--no-sandbox','--disable-dev-shm-usage']});
try{
 const page=await browser.newPage({viewport:{width:1600,height:900},deviceScaleFactor:1});
 await page.route('http**://**/*',route=>route.abort());
 await page.setContent(html);
 await page.evaluate(()=>Promise.all([...document.images].map(image=>image.decode())));
 await page.screenshot({path:path.join(out,'banner.png')});
}finally{await browser.close();}
console.log('Publication banner and six model captures prepared. These are studio assets, not retail screenshots.');
