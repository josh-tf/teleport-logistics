// Compose listing artwork from the shipped model captures, without external requests.
import {chromium} from 'playwright';
import {readFile,mkdir,copyFile} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const out=path.join(root,'docs/publishing/media');
await mkdir(out,{recursive:true});
const models=[['ItemInput','Item Input'],['ItemOutput','Item Output'],['FluidInput','Fluid Input'],['FluidOutput','Fluid Output'],['Hub','Network Hub'],['TravelHub','Personnel']];
const cards=[];
for(const [name,label] of models){
 const source=path.join(root,`assets/icons/T_Teleporter${name}_512.png`);
 await copyFile(source,path.join(out,`${name}.png`));
 const data=(await readFile(source)).toString('base64');
 cards.push(`<article><img alt="${label}" src="data:image/png;base64,${data}"><p>${label}</p></article>`);
}
await copyFile(path.join(root,'assets/icons/T_TeleporterMilestone_512.png'),path.join(out,'icon-512.png'));
const html=`<!doctype html><html lang="en"><meta charset="utf-8"><title>Teleport Logistics model preview</title><style>
*{box-sizing:border-box}body{margin:0;width:1600px;height:900px;background:#181d22;color:#f1f3f5;font-family:Arial,sans-serif;padding:76px 70px;position:relative}h1{font-size:84px;letter-spacing:2px;margin:0 0 18px}h2{font-size:30px;font-weight:400;color:#d2d7da;margin:0}header{border-left:8px solid #ee9e43;padding-left:32px}section{display:grid;grid-template-columns:repeat(6,1fr);gap:12px;margin-top:115px}article{background:#222a31;border-top:2px solid #3b454e;border-radius:6px;padding:25px 4px;text-align:center}img{width:220px;height:230px;object-fit:contain}p{font-size:23px;margin:14px 0 5px}footer{position:absolute;bottom:64px;left:70px;right:70px;display:flex;justify-content:space-between;border-top:1px solid #3b454e;padding-top:28px;color:#b6c0c7;font-size:23px}b{color:#ee9e43;font-weight:400}
</style><header><h1>TELEPORT LOGISTICS</h1><h2>Named routes for your factory. Personal travel for your Pioneer.</h2></header><section>${cards.join('')}</section><footer><span><b>TIER 5</b> Logistics &nbsp; · &nbsp; <b>TIER 9</b> Personnel</span><span>Unofficial Satisfactory mod · Model preview</span></footer></html>`;
const browser=await chromium.launch({headless:true,executablePath:process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE || chromium.executablePath(),args:['--no-sandbox','--disable-dev-shm-usage']});
try{
 const page=await browser.newPage({viewport:{width:1600,height:900},deviceScaleFactor:1});
 await page.route('http**://**/*',route=>route.abort());
 await page.setContent(html);
 await page.evaluate(()=>Promise.all([...document.images].map(image=>image.decode())));
 await page.screenshot({path:path.join(out,'banner.png')});
}finally{await browser.close();}
console.log('Publication banner and six model captures prepared. These are studio assets, not retail screenshots.');
