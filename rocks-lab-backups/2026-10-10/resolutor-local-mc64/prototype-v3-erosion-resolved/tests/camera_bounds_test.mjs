import {validCamera} from '../viewer/app.js';
const c={schemaVersion:1,target:[1e308,1e308,1e308],yaw:0,pitch:.3,distance:5,fov:.7};if(validCamera(c))throw Error('Extreme finite camera target accepted');console.log('Camera target bounds PASS');
