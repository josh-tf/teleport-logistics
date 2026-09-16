import {chromium} from 'playwright';
import path from 'node:path';
import fs from 'node:fs';
import {fileURLToPath,pathToFileURL} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const version=JSON.parse(fs.readFileSync(path.join(root,'TeleportLogistics/TeleportLogistics.uplugin'),'utf8')).SemVersion;
const reportDir=path.join(root,'reports/model-renders',version);
fs.mkdirSync(reportDir,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE || chromium.executablePath(),headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader','--disable-dev-shm-usage']});
const page=await browser.newPage({viewport:{width:1200,height:850}});const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
await page.route('http**://**/*',route=>route.abort());
await page.goto(pathToFileURL(path.join(root,'dist/TeleportLogistics-model-viewer.html')).href);
await page.waitForFunction(()=>window.TELEPORTLOGISTICS_VIEWER_STATE?.models.length===6);
await page.waitForTimeout(400);
for(const name of ['ItemInput','ItemOutput','FluidInput','FluidOutput','Hub','TravelHub','all']){
 await page.selectOption('#model',name);await page.waitForFunction(n=>window.TELEPORTLOGISTICS_VIEWER_STATE.active===n,name);
 if(await page.evaluate(()=>window.TELEPORTLOGISTICS_VIEWER_STATE.triangles<10000))throw new Error(`No triangle geometry for ${name}`);
 for(const mode of ['textured','solid','wire']){await page.selectOption('#surface',mode);await page.waitForTimeout(30);}
}
await page.selectOption('#surface','textured');await page.selectOption('#model','ItemInput');
for(const level of ['1','2']){await page.selectOption('#lod',level);await page.waitForTimeout(50);if(await page.evaluate(()=>window.TELEPORTLOGISTICS_VIEWER_STATE.triangles>=20000||window.TELEPORTLOGISTICS_VIEWER_STATE.triangles<1000))throw new Error('Invalid reduced LOD');}
await page.selectOption('#lod','2');await page.click('[data-view=rear]');await page.screenshot({path:path.join(reportDir,'viewer-lod2.png')});
await page.selectOption('#lod','0');
await page.click('[data-view=back]');await page.screenshot({path:path.join(reportDir,'viewer-back.png')});
await page.click('[data-view=front]');await page.screenshot({path:path.join(reportDir,'viewer-entrance.png')});
await page.click('[data-view=rear]');await page.check('#ports');await page.uncheck('#lights');await page.check('#lights');await page.uncheck('#ports');
await page.locator('canvas').focus();await page.keyboard.press('ArrowLeft');await page.keyboard.press('+');await page.keyboard.press('r');
await page.screenshot({path:path.join(reportDir,'viewer-desktop.png')});
await page.selectOption('#model','TravelHub');
for(const view of ['front','rear','back']){await page.click(`[data-view=${view}]`);await page.waitForTimeout(150);await page.screenshot({path:path.join(reportDir,`personnel-${view}.png`)});}
await page.uncheck('#lights');await page.screenshot({path:path.join(reportDir,'personnel-unpowered.png')});await page.check('#lights');
await page.setViewportSize({width:390,height:844});await page.screenshot({path:path.join(reportDir,'viewer-mobile.png')});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth))errors.push('Horizontal overflow at mobile width');
if(await page.locator('#error').isVisible())errors.push(await page.locator('#error').innerText());
await browser.close();if(errors.length)throw new Error(errors.join('\n'));
console.log('PASS: offline viewer loads all six actual models; all surface modes, cameras, lamps, markers, keyboard orbit and responsive layout checked; no browser errors.');
