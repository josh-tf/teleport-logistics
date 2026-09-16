import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';
const file=fileURLToPath(new URL('../../dist/TeleportLogistics-model-viewer.html',import.meta.url));
createServer(async(req,res)=>{
 if(req.url!=='/'&&req.url!=='/TeleportLogistics-model-viewer.html'){res.writeHead(404);res.end();return;}
 try{const html=await readFile(file);res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store'});res.end(html);}catch{res.writeHead(500);res.end('Viewer has not been built.');}
}).listen(8765,'127.0.0.1',()=>console.log('TeleportLogistics viewer: http://127.0.0.1:8765'));
