import unittest
import test_lab

def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def pierces(a,b,tri):
    direction=sub(b,a);e1=sub(tri[1],tri[0]);e2=sub(tri[2],tri[0]);h=cross(direction,e2);det=dot(e1,h)
    if abs(det)<1e-10:return False
    s=sub(a,tri[0]);u=dot(s,h)/det
    if u<=1e-8 or u>=1-1e-8:return False
    q=cross(s,e1);v=dot(direction,q)/det
    if v<=1e-8 or u+v>=1-1e-8:return False
    t=dot(e2,q)/det
    return 1e-8<t<1-1e-8

class PartialLayerTests(unittest.TestCase):
    generate=test_lab.LabTests.generate
    def test_third_layer_partial_patches_do_not_self_intersect(self):
        out,_=self.generate('corridor',layers=3,sag=.55,tension=.4,quality=20,density=0,atlas=64)
        vertices=[];faces=[]
        for line in (out/'mesh.obj').read_text().splitlines():
            parts=line.split()
            if parts and parts[0]=='v':vertices.append(tuple(map(float,parts[1:])))
            if parts and parts[0]=='f':faces.append(tuple(int(v.split('/')[0])-1 for v in parts[1:]))
        # Nine quads of 800 faces, then six partial triangles of 400 faces.
        for patch in (12,13,14):
            start=7200+(patch-9)*400;selected=faces[start:start+400]
            tris=[[vertices[i] for i in face] for face in selected]
            boxes=[(tuple(min(v[k] for v in tri) for k in range(3)),tuple(max(v[k] for v in tri) for k in range(3))) for tri in tris]
            for i,a in enumerate(selected):
                for j in range(i+1,len(selected)):
                    if set(a).intersection(selected[j]):continue
                    if any(boxes[i][1][k]<boxes[j][0][k] or boxes[j][1][k]<boxes[i][0][k] for k in range(3)):continue
                    left,right=tris[i],tris[j]
                    hit=any(pierces(left[k],left[(k+1)%3],right) or pierces(right[k],right[(k+1)%3],left) for k in range(3))
                    self.assertFalse(hit,f'patch {patch}: nonadjacent faces {start+i}/{start+j} intersect')
if __name__=='__main__':unittest.main()
