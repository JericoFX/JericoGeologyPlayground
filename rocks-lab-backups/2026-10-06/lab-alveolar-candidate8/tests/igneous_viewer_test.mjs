import fs from 'node:fs';import assert from 'node:assert/strict';
const source=fs.readFileSync(new URL('../viewer/app.js',import.meta.url),'utf8').replace(/^import.*\n/,'').replace('function validRecipe(', 'export function validRecipe(');
const {validRecipe,familyRecipe}=await import('data:text/javascript,'+encodeURIComponent(source));
const original=JSON.parse(fs.readFileSync(new URL('../recipes/heightfield-cliff-48-v10.json',import.meta.url)));
for(const family of ['columnar_basalt','jointed_granite']){
 const expected=JSON.parse(fs.readFileSync(new URL(`../recipes/${family}-48-v12.json`,import.meta.url)));
 const result=familyRecipe(original,family,1);assert.equal(result.generatorVersion,'rocks-v12');assert.equal(result.family,family);assert.equal(result.detail,0);assert.ok(validRecipe(result));assert.ok(validRecipe(expected));const optional={...expected};delete optional.detail;assert.ok(validRecipe(optional));const thin=familyRecipe({...expected,layerThickness:.04},'jointed_granite',0);assert.ok(validRecipe(thin));
 assert.equal(validRecipe({...result,unexpected:1}),false);assert.equal(validRecipe({...result,studySpacing:.1}),false);assert.equal(validRecipe({...result,wear:1}),false);assert.equal(validRecipe({...result,detail:1}),false);
 const selected=familyRecipe({...result,studySpacing:.51},family,0);assert.equal(selected.studySpacing,.51);
 const old=familyRecipe(selected,'weathered_boulder',0);assert.ok(validRecipe(old));for(const key of ['studySpacing','columnCount','columnLean','jointDepth','roundness','chipping'])assert.equal(key in old,false);
 const other=familyRecipe(selected,family==='columnar_basalt'?'jointed_granite':'columnar_basalt',0);assert.ok(validRecipe(other));assert.equal(other.studySpacing,.45);
}
console.log('v12 exact family controls, import validation, switch cleanup PASS (no WebGL)');
