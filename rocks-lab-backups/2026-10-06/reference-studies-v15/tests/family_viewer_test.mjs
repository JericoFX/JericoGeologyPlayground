import fs from 'node:fs';
const source=fs.readFileSync(new URL('../viewer/app.js',import.meta.url),'utf8').replace(/^import.*\n/,'').replace('function validRecipe(', 'export function validRecipe(');
const {validRecipe,familyRecipe}=await import('data:text/javascript,'+encodeURIComponent(source));
for(const family of ['common_rock','cliff','strata']){
 const r=JSON.parse(fs.readFileSync(new URL(`../recipes/${family}-48-v4.json`,import.meta.url)));
 if(!validRecipe(r))throw Error('Viewer rejects '+family);
 if(!validRecipe({...r,generatorVersion:'rocks-v5'}))throw Error('Viewer rejects v5 '+family);
 if(validRecipe({...r,generatorVersion:'rocks-v3'}))throw Error('Viewer accepts mismatched generator');
}
console.log('v4 viewer recipe acceptance and version rejection PASS (no WebGL).');

const sample=JSON.parse(fs.readFileSync(new URL('../recipes/common_rock-48-v5.json',import.meta.url)));
const weathered=familyRecipe(sample,'weathered_boulder',1),plateau=familyRecipe(sample,'cliff_plateau',1);
if(weathered.generatorVersion!=='rocks-v7'||weathered.extractorVersion!=='paris-mc-v1'||!validRecipe(weathered))throw Error('Weathered selection does not use MC v7');
if(plateau.generatorVersion!=='rocks-v8'||plateau.detail!==0||!validRecipe(plateau))throw Error('Plateau selection does not use bounded v8');
if(validRecipe({...plateau,extractorVersion:'paris-mc-v1'}))throw Error('Plateau MC route must reject');
