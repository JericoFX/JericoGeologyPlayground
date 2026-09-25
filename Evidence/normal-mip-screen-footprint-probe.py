import sys, math, json
from pathlib import Path
import numpy as np

def inspect(path, resolution, distance, yaw=.5, pitch=-.28, width=1280, height=960):
    vertices=[]; uvs=[]; faces=[]; faceuv=[]
    for line in path.read_text().splitlines():
        if line.startswith('v '): vertices.append([float(x) for x in line.split()[1:4]])
        elif line.startswith('vt '): uvs.append([float(x) for x in line.split()[1:3]])
        elif line.startswith('f '):
            pairs=[x.split('/') for x in line.split()[1:4]]
            faces.append([int(x[0])-1 for x in pairs]); faceuv.append([int(x[1])-1 for x in pairs])
    p=np.asarray(vertices)[np.asarray(faces)]
    uv=np.asarray(uvs)[np.asarray(faceuv)]
    cy,sy,cp,sp=math.cos(yaw),math.sin(yaw),math.cos(pitch),math.sin(pitch)
    right=np.asarray([cy,0,-sy]); forward=np.asarray([sy*cp,sp,cy*cp]); up=np.cross(forward,right)
    depth=p@forward+distance
    sx=((p@right)*2.4/(width/height)/depth*.5+.5)*width
    syy=(.5-(p@up)*2.4/depth*.5)*height
    s=np.stack((sx,syy),axis=2)
    x1=s[:,1]-s[:,0]; x2=s[:,2]-s[:,0]
    det=x1[:,0]*x2[:,1]-x1[:,1]*x2[:,0]
    area=np.abs(det)*.5
    # Gradient of screen barycentric weights for vertices 1 and 2.
    ok=(np.min(depth,axis=1)>0) & (area>1e-4) & (np.max(s[:,:,0],axis=1)>=0) & (np.min(s[:,:,0],axis=1)<width) & (np.max(s[:,:,1],axis=1)>=0) & (np.min(s[:,:,1],axis=1)<height)
    p=p[ok]; uv=uv[ok]; depth=depth[ok]; x1=x1[ok]; x2=x2[ok]; det=det[ok]; area=area[ok]
    g1=np.stack((x2[:,1],-x2[:,0]),axis=1)/det[:,None]
    g2=np.stack((-x1[:,1],x1[:,0]),axis=1)/det[:,None]
    g0=-g1-g2
    grads=np.stack((g0,g1,g2),axis=1)
    invw=1/depth
    B=np.mean(invw,axis=1)
    A=np.mean(uv*invw[:,:,None],axis=1)
    u=A/B[:,None]
    dA=np.einsum('fij,fik->fjk',grads,uv*invw[:,:,None])
    dB=np.einsum('fij,fi->fj',grads,invw)
    deriv=(dA-u[:,None,:]*dB[:,:,None])/B[:,None,None]
    rho=np.max(np.linalg.norm(deriv*resolution,axis=2),axis=1)
    lod=np.log2(np.maximum(rho,1e-9))
    front=np.einsum('fi,i->f',np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0]),-forward)>0
    out={'path':str(path),'resolution':resolution,'distance':distance,'faces':len(lod),'frontFaces':int(front.sum())}
    for label,mask in (('all',np.ones_like(front,dtype=bool)),('front',front)):
        x=lod[mask]; w=area[mask]; order=np.argsort(x); x=x[order]; w=w[order]; c=np.cumsum(w)/w.sum()
        out[label]={'p50':float(x[np.searchsorted(c,.5)]),'p95':float(x[np.searchsorted(c,.95)]),'fractionAreaAboveMip3':float(w[x>3].sum()/w.sum()),'fractionAreaAboveMip4':float(w[x>4].sum()/w.sum())}
    return out
for arg in sys.argv[1:]:
    name,rez,dist=arg.split(',')
    print(json.dumps(inspect(Path(name),int(rez),float(dist)),indent=2))
