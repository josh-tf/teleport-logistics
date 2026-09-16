import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { OBJLoader } from 'three/addons/loaders/OBJLoader.js';
const $ = id => document.getElementById(id);
try {
const stage=$('stage'), renderer=new THREE.WebGLRenderer({antialias:true,alpha:false});
renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));
renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.25;
stage.prepend(renderer.domElement);renderer.domElement.setAttribute('aria-label','TeleportLogistics model. Drag to orbit; arrow keys rotate; plus and minus zoom.');renderer.domElement.tabIndex=0;
const scene=new THREE.Scene();scene.background=new THREE.Color('#171b20');
const camera=new THREE.PerspectiveCamera(38,1,.02,500);camera.up.set(0,0,1);
const controls=new OrbitControls(camera,renderer.domElement);controls.enableDamping=false;controls.minDistance=.15;controls.maxDistance=100;
scene.add(new THREE.HemisphereLight(0xd7e8ff,0x565045,2.1));
for(const [pos,power,color] of [[[5,-7,9],3.2,0xffffff],[[-4,5,6],2.3,0xcfe7ff],[[6,5,4],1.7,0xffd3a2]]){const l=new THREE.DirectionalLight(color,power);l.position.set(...pos);scene.add(l);}
const floor=new THREE.GridHelper(40,40,0x697079,0x383f47);floor.rotation.x=Math.PI/2;floor.position.z=-.015;scene.add(floor);
const group=new THREE.Group();scene.add(group);const models={}, metadata={}, loader=new OBJLoader();
const paint={primary:{value:new THREE.Color($('primary').value)},secondary:{value:new THREE.Color($('secondary').value)}};
const materials=[];const textureLoader=new THREE.TextureLoader();
function texture(data){const t=textureLoader.load(data,draw,undefined,()=>fail('A model texture could not be loaded.'));t.colorSpace=THREE.SRGBColorSpace;t.anisotropy=Math.min(4,renderer.capabilities.getMaxAnisotropy());return t;}
const details=texture(window.TELEPORTLOGISTICS_DATA.textures.details);
const atlas=texture(window.TELEPORTLOGISTICS_DATA.textures.factory),screen=texture(window.TELEPORTLOGISTICS_DATA.textures.screen);
function factoryMaterial(name){
 const isScreen=name==='screen',signal=name==='signal';
 const mat=new THREE.MeshStandardMaterial({color:0xffffff,map:isScreen?screen:atlas,metalness:isScreen?0:.34,roughness:.42,side:THREE.FrontSide});
 if(isScreen||signal){mat.emissive.set(0xffffff);mat.emissiveMap=isScreen?screen:atlas;mat.emissiveIntensity=isScreen?.35:1.1;}
 mat.userData={signal,isScreen,originalMap:mat.map,emission:mat.emissiveIntensity};
 if(name==='factory')mat.onBeforeCompile=shader=>{
  shader.uniforms.teleporterPrimary=paint.primary;shader.uniforms.teleporterSecondary=paint.secondary;
  shader.fragmentShader='uniform vec3 teleporterPrimary; uniform vec3 teleporterSecondary;\n'+shader.fragmentShader;
  shader.fragmentShader=shader.fragmentShader.replace('#include <map_fragment>',`#include <map_fragment>
   #ifdef USE_MAP
   if(vMapUv.x<0.333 && vMapUv.y>0.667) diffuseColor.rgb=teleporterPrimary;
   else if(vMapUv.x<0.333 && vMapUv.y>0.333 && vMapUv.y<0.667) diffuseColor.rgb=teleporterSecondary;
   #endif`);
 };
 if(name==='decal_custom'){mat.map=details;mat.userData.originalMap=details;mat.alphaTest=.5;mat.roughness=.65;}
 if(name.startsWith('decal')&&name!=='decal_custom'){mat.map=null;mat.color.set('#343940');mat.opacity=0;mat.transparent=true;mat.depthWrite=false;}
 materials.push(mat);return mat;
}
for(const [name,text] of Object.entries(window.TELEPORTLOGISTICS_DATA.models)){
 // OBJ exports contain a few loose construction edges alongside the faces.
 // OBJLoader otherwise classifies the combined object as LineSegments.
 const obj=loader.parse(text.split('\n').filter(line=>!/^\s*[lp]\s/.test(line)).join('\n'));obj.scale.setScalar(.01);let triangles=0;
 obj.traverse(child=>{if(!child.isMesh)return;const old=Array.isArray(child.material)?child.material:[child.material];child.material=old.map(m=>factoryMaterial(m.name));triangles+=child.geometry.attributes.position.count/3;});
 obj.updateMatrixWorld(true);const b=new THREE.Box3().setFromObject(obj),size=b.getSize(new THREE.Vector3());
 if(triangles<10000)throw new Error(`Incomplete triangle mesh: ${name}`);
 metadata[name]={triangles,size,lodCounts:[triangles]};models[name]=obj;
 obj.userData.lods=[obj];
 for(const source of window.TELEPORTLOGISTICS_DATA.lods[name]){
  const lod=loader.parse(source.split('\n').filter(line=>!/^\s*[lp]\s/.test(line)).join('\n'));lod.scale.setScalar(.01);let count=0;
  lod.traverse(child=>{if(!child.isMesh)return;const old=Array.isArray(child.material)?child.material:[child.material];child.material=old.map(m=>factoryMaterial(m.name));count+=child.geometry.attributes.position.count/3;});
  obj.userData.lods.push(lod);metadata[name].lodCounts.push(count);
 }
 const markers=new THREE.Group();markers.name='ports';
 // OBJ coordinates are Blender Z-up. Unreal importer mirrors Y.
 const coords=name==='TravelHub'?[-1.8,-2.2,3.1]:name==='Hub'?[-.8364,-.525,2.988]:[1.60,0,name.startsWith('Fluid')?1.75:1.00];
 const dot=new THREE.Mesh(new THREE.SphereGeometry(.055,12,8),new THREE.MeshBasicMaterial({color:0xff62c6,depthTest:false}));dot.position.set(...coords);dot.renderOrder=100;
 const arrow=new THREE.ArrowHelper(new THREE.Vector3(name==='Hub'?-1:1,0,0),new THREE.Vector3(...coords),.55,0xff62c6,.12,.07);markers.add(dot,arrow);obj.userData.markers=markers;
}
let active='ItemInput',view='rear',radius=4;
const views={rear:[-1.55,1.75,.8],hero:[1.55,-1.75,1.22],back:[-1,0,.12],front:[1,0,.1],left:[0,-1,.12],right:[0,1,.12],top:[.001,0,1],bottom:[.001,0,-1]};
function bounds(){return new THREE.Box3().setFromObject(group);}
function fit(){const b=bounds(),c=b.getCenter(new THREE.Vector3()),s=b.getSize(new THREE.Vector3());radius=s.length()*.5;controls.target.copy(c);const distance=radius/Math.sin(THREE.MathUtils.degToRad(camera.fov*.5))/Math.min(1,camera.aspect)*1.08;camera.position.copy(c).addScaledVector(new THREE.Vector3(...views[view]).normalize(),distance);camera.near=Math.max(.01,radius/500);camera.far=500;camera.updateProjectionMatrix();controls.update();draw();}
function show(){group.clear();active=$('model').value;const names=active==='all'?Object.keys(models):[active];let x=0;
 for(const name of names){const obj=models[name].userData.lods[Number($('lod').value)];obj.position.set(x,0,0);group.add(obj);const markers=models[name].userData.markers;markers.position.set(x,0,0);markers.visible=$('ports').checked;group.add(markers);x+=Math.max(metadata[name].size.x,4)+2;}
 const stats=names.map(n=>metadata[n]);const count=stats.reduce((a,b)=>a+b.lodCounts[Number($('lod').value)],0);$('readout').textContent=active==='all'?`All six at the same scale · ${count.toLocaleString()} triangles · Grid: 1 m`:`${$('model').selectedOptions[0].text} · ${count.toLocaleString()} triangles · ${stats[0].size.x.toFixed(2)} m deep × ${stats[0].size.y.toFixed(2)} m wide × ${stats[0].size.z.toFixed(2)} m tall`;
 fit();$('status').textContent='6 assets ready';window.TELEPORTLOGISTICS_VIEWER_STATE={active,triangles:count,models:Object.keys(models)};
}
function draw(){renderer.render(scene,camera);}
function fail(message){$('error').hidden=false;$('error').textContent=message;$('status').textContent='Viewer error';}
controls.addEventListener('change',draw);
function resize(){const r=stage.getBoundingClientRect();renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();draw();}new ResizeObserver(resize).observe(stage);
$('model').onchange=show;$('lod').onchange=show;
for(const button of document.querySelectorAll('[data-view]'))button.onclick=()=>{view=button.dataset.view;document.querySelectorAll('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));fit();};
$('fit').onclick=fit;$('grid').onchange=()=>{floor.visible=$('grid').checked;draw();};$('ports').onchange=()=>{for(const m of Object.values(models))m.userData.markers.visible=$('ports').checked;draw();};
for(const key of ['primary','secondary'])$(key).oninput=()=>{paint[key].value.set($(key).value);draw();};
function surface(){const mode=$('surface').value,on=$('lights').checked;for(const m of materials){if(m.transparent)continue;m.wireframe=mode==='wire';m.map=mode==='textured'?m.userData.originalMap:null;m.color.set(mode==='textured'?0xffffff:0x969da4);m.emissiveIntensity=mode==='textured'&&on?m.userData.emission:0;if(m.userData.isScreen&&!on)m.color.set(0x07090b);m.needsUpdate=true;}draw();}
$('surface').onchange=surface;$('lights').onchange=surface;
$('fullscreen').onclick=async()=>{try{if(document.fullscreenElement)await document.exitFullscreen();else await document.documentElement.requestFullscreen();}catch{fail('Full screen is unavailable in this browser panel.');}};
renderer.domElement.addEventListener('keydown',e=>{const delta=camera.position.clone().sub(controls.target);if(e.key==='r'||e.key==='R'){fit();return;}if(e.key==='+'||e.key==='=')delta.multiplyScalar(.85);else if(e.key==='-')delta.multiplyScalar(1.18);else if(['ArrowLeft','ArrowRight'].includes(e.key))delta.applyAxisAngle(new THREE.Vector3(0,0,1),e.key==='ArrowLeft'?.12:-.12);else if(['ArrowUp','ArrowDown'].includes(e.key)){const right=new THREE.Vector3().crossVectors(delta,camera.up).normalize();delta.applyAxisAngle(right,e.key==='ArrowUp'?.12:-.12);}else return;e.preventDefault();camera.position.copy(controls.target).add(delta);controls.update();draw();});
renderer.domElement.addEventListener('webglcontextlost',e=>{e.preventDefault();fail('3D rendering was interrupted. Reload the viewer to restore it.');});
const requested=decodeURIComponent(location.hash.slice(1));if(models[requested])$('model').value=requested;
resize();show();
}catch(error){$('error').hidden=false;$('error').textContent=`Unable to start 3D viewer: ${error.message}`;$('status').textContent='Viewer error';console.error(error);}
