import fs from 'node:fs';
const source=fs.readFileSync(new URL('../viewer/app.js',import.meta.url),'utf8').replace(/^import.*\n/,'').replace('function validRecipe(', 'export function validRecipe(');
const {validRecipe,familyRecipe}=await import('data:text/javascript,'+encodeURIComponent(source));
const recipe=JSON.parse(fs.readFileSync(new URL('../recipes/heightfield-cliff-48-v10.json',import.meta.url)));
if(!validRecipe(recipe))throw Error('Heightfield route rejected');
for(const [key,value] of [['seed',-1],['fractures',true],['tiltDegrees',46],['unknown',0],['grid',{cells:128,width:512,height:512,extra:1}]])if(validRecipe({...recipe,[key]:value}))throw Error('Malformed accepted: '+key);
if(JSON.stringify(familyRecipe(recipe,'heightfield_cliff',0))!==JSON.stringify(recipe))throw Error('Imported exact heightfield recipe altered');
const legacy=familyRecipe(recipe,'stratified',0);if(!validRecipe(legacy)||'fractures' in legacy)throw Error('Heightfield to legacy conversion invalid');
const converted=familyRecipe(legacy,'heightfield_cliff',0);if(!validRecipe(converted)||'wear' in converted)throw Error('Legacy to heightfield conversion invalid');
console.log('Heightfield validation, exact preservation and route switching PASS (Node, no WebGL).');

for(const name of ['massive-oblique','vertical-channels']){
 const r=JSON.parse(fs.readFileSync(new URL('../recipes/heightfield-'+name+'-48-v11.json',import.meta.url)));
 if(!validRecipe(r))throw Error('New profile rejected');
 if(JSON.stringify(familyRecipe(r,'heightfield_cliff',0))!==JSON.stringify(r))throw Error('Profile import altered');
 for(const [key,value] of [['profile','unknown'],['jointSpacing',false],['directionDegrees',91],['jointDepth',-1],['blockScale',2],['bedCount',12]])if(validRecipe({...r,[key]:value}))throw Error('Malformed profile accepted '+key);
}
