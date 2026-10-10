import fs from 'node:fs';
import assert from 'node:assert/strict';
const root=new URL('..',import.meta.url).pathname.replace(/\/$/,'');
const recipe=JSON.parse(fs.readFileSync(root+'/recipes/heightfield-cliff-48-v10.json'));
const elements=new Map();const el=k=>{if(!elements.has(k))elements.set(k,{value:'',textContent:'',checked:false,dataset:{},replaceChildren(...children){this.children=children},append(){},click(){}});return elements.get(k)};
let sent=[],modes=[],meshes=[];const storage=new Map();
globalThis.document={querySelector:el,querySelectorAll:()=>[],body:{dataset:{}},createElement:()=>({click(){}})};
globalThis.window={addEventListener(){}};globalThis.localStorage={getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v)};
globalThis.__renderer=()=>({setCamera(){},setMode(m){modes.push(m)},setMesh(m){meshes.push(m);return 0},draw(){}});
const res=data=>({ok:true,json:async()=>data,text:async()=>JSON.stringify(data)});
const view={domain:{min:[-1,-1,-.3],max:[1,1,.9]},stages:{heightfield:{positions:[0,0,0],indices:[]}}};
const metrics={clippedLow:0,clippedHigh:0,stages:{heightfield:{vertices:1,triangles:0,signedVolume:1}}};
globalThis.fetch=async(url,options)=>{if(url.includes('manifest'))return res({materials:[]});if(url==='/api/current')return {ok:false};if(url==='/default-recipe.json')return res(recipe);if(url==='/api/generate'){const data=JSON.parse(options.body);sent.push(data);return res({requestId:data.requestId,viewerUrl:'/view',metricsUrl:'/metrics',exports:{heightfield:'/heightfield.obj',recipe:'/recipe.json'}})}return res(url==='/view'?view:metrics)};
el('#mode').value='resistance';
const source=fs.readFileSync(root+'/viewer/app.js','utf8').replace(/^import.*\n/,'const createRenderer=globalThis.__renderer;\n');await import('data:text/javascript,'+encodeURIComponent(source));
for(let i=0;i<10&&!window.labEvidence;i++)await new Promise(r=>setTimeout(r,0));
assert.deepEqual(window.labEvidence.recipe,recipe);assert.equal(document.body.dataset.heightfield,'true');assert.equal(el('#legacyControls').hidden,true);assert.equal(el('#heightfieldControls').hidden,false);assert.equal(el('#resistanceOption').disabled,true);assert.equal(el('#mode').value,'neutral');assert.deepEqual(modes,['neutral','neutral','neutral']);assert.equal(meshes.length,1);assert.equal(el('#downloads').children.length,2);
for(const [key,value] of [['seed',117],['size',3],['bedCount',6],['thicknessVariation',.2],['tiltDegrees',20],['relief',.45],['backDepth',.5],['fractures',.3],['irregularity',.4]]){const before=structuredClone(window.labEvidence.recipe);el('#'+key).value=String(value);await el('#generate').onclick();assert.deepEqual(sent.at(-1).recipe,{...before,[key]:value})}
el('#cells').value='64';el('#sampleWidth').value='256';el('#sampleHeight').value='128';el('#fragmentedContacts').checked=true;await el('#generate').onclick();assert.deepEqual(sent.at(-1).recipe.grid,{cells:64,width:256,height:128});assert.equal(sent.at(-1).recipe.fragmentedContacts,true);
await el('#importRecipe').onchange({target:{files:[{text:async()=>JSON.stringify(recipe)}]}});assert.deepEqual(window.labEvidence.recipe,recipe);
const requests=sent.length;await el('#importRecipe').onchange({target:{files:[{text:async()=>JSON.stringify({...recipe,fractures:2})}]}});assert.equal(sent.length,requests);assert.deepEqual(window.labEvidence.recipe,recipe);
el('#cliffProfile').value='massive_oblique';el('#cliffProfile').onchange();el('#jointDepth').value='.25';el('#cliffProfile').value='vertical_channels';el('#cliffProfile').onchange();assert.equal(Number(el('#jointDepth').value),.25,'Pending profile switch preserves edited joint controls');el('#cliffProfile').value='stratified';el('#cliffProfile').onchange();
el('#cliffProfile').value='massive_oblique';el('#cliffProfile').onchange();assert.equal(el('#beddingControls').hidden,true);assert.equal(el('#jointControls').hidden,false);await el('#generate').onclick();assert.equal(sent.at(-1).recipe.profile,'massive_oblique');assert.equal(sent.at(-1).recipe.generatorVersion,'heightfield-cliff-v11');assert.equal('bedCount' in sent.at(-1).recipe,false);
for(const [key,value] of [['directionDegrees',25],['jointSpacing',.16],['jointDepth',.8],['blockScale',.4]]){const before=structuredClone(window.labEvidence.recipe);el('#'+key).value=String(value);await el('#generate').onclick();assert.deepEqual(sent.at(-1).recipe,{...before,[key]:value})}
el('#cliffProfile').value='vertical_channels';el('#cliffProfile').onchange();await el('#generate').onclick();assert.equal(sent.at(-1).recipe.profile,'vertical_channels');
const jointRecipe=structuredClone(window.labEvidence.recipe);await el('#importRecipe').onchange({target:{files:[{text:async()=>JSON.stringify(jointRecipe)}]}});assert.deepEqual(window.labEvidence.recipe,jointRecipe);
el('#cliffProfile').value='stratified';el('#cliffProfile').onchange();await el('#generate').onclick();assert.equal(sent.at(-1).recipe.generatorVersion,'heightfield-cliff-v10');assert.equal('jointSpacing' in sent.at(-1).recipe,false);assert.equal(el('#beddingControls').hidden,false);
const oldLabel=el('#base-title').textContent;el('#family').value='stratified';el('#family').onchange();assert.equal(document.body.dataset.heightfield,'true','Selecting an ungenerated route must not relabel old geometry');assert.equal(el('#base-title').textContent,oldLabel);assert.equal(el('#heightfieldControls').hidden,true);
console.log('Heightfield UI controls, route labels, exact import, invalid retention PASS (mocked DOM, no WebGL).');
