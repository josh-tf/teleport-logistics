import {build} from 'esbuild';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),root=path.resolve(here,'../..');
const version=JSON.parse(await readFile(path.join(root,'TeleportLogistics/TeleportLogistics.uplugin'),'utf8')).SemVersion;
const models={};for(const name of ['ItemInput','ItemOutput','FluidInput','FluidOutput','Hub','TravelHub'])models[name]=await readFile(path.join(root,`assets/models/SM_Teleporter${name}.obj`),'utf8');
const lods={};for(const name of Object.keys(models)){lods[name]=[];for(const level of [1,2])lods[name].push(await readFile(path.join(root,`assets/models/lods/SM_Teleporter${name}_LOD${level}.obj`),'utf8'));}
async function image(p){return 'data:image/png;base64,'+(await readFile(path.join(root,p))).toString('base64');}
const textures={details:await image('assets/textures/T_TeleporterDetails.png'),factory:await image('assets/vendor/Satisfactory_ModelingTools/Factory_Base_Plain.png'),screen:await image('assets/textures/T_TeleporterScreen_Grid.png')};
const js=await build({entryPoints:[path.join(here,'viewer.js')],bundle:true,write:false,minify:true,format:'iife',legalComments:'inline'});
const data=JSON.stringify({models,lods,textures}).replaceAll('<','\\u003c');
const license=await readFile(path.join(here,'node_modules/three/LICENSE'),'utf8');
const notices=await readFile(path.join(root,'TeleportLogistics/Resources/NAMEPLATE-NOTICE.md'),'utf8');
const credits=JSON.stringify({three:license,nameplates:notices}).replaceAll('<','\\u003c');
const html=(await readFile(path.join(here,'index.html'),'utf8')).replace('<!--TELEPORTLOGISTICS_VERSION-->',version).replace('<!--TELEPORTLOGISTICS_DATA-->',`<script type="application/json" id="third-party-notices">${credits}</script><script>window.TELEPORTLOGISTICS_DATA=${data};</script>`).replace('/*TELEPORTLOGISTICS_BUNDLE*/',()=>js.outputFiles[0].text.replaceAll('</script','<\\/script'));
await mkdir(path.join(root,'dist'),{recursive:true});await writeFile(path.join(root,'dist/TeleportLogistics-model-viewer.html'),html);console.log('Built dist/TeleportLogistics-model-viewer.html: all six meshes, embedded textures, offline JS');
