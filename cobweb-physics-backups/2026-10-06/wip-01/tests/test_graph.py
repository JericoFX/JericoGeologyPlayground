import json,pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parent))
import test_lab
class GraphTests(unittest.TestCase):
    generate=test_lab.LabTests.generate
    def test_each_patch_graph_is_connected_to_its_fixed_boundary(self):
        for preset in ('radial','corner','corridor'):
            out,_=self.generate(preset,density=.25)
            data=json.loads((out/'curves.json').read_text()); nodes={n['id']:n for n in data['nodes']}; adj={}
            for f in data['curves']:
                a,b=f['node_a'],f['node_b']; self.assertNotEqual(a,b)
                self.assertEqual(nodes[a]['patch'],f['patch']); self.assertEqual(nodes[b]['patch'],f['patch'])
                for got,want in ((f['uv'][0],nodes[a]['uv']),(f['uv'][-1],nodes[b]['uv'])):
                    self.assertLess(sum((got[i]-want[i])**2 for i in range(2)),1e-15)
                adj.setdefault(a,set()).add(b);adj.setdefault(b,set()).add(a)
            for patch in set(n['patch'] for n in nodes.values()):
                members={n['id'] for n in nodes.values() if n['patch']==patch and n['id'] in adj}
                start=min(members); seen={start}; todo=[start]
                while todo:
                    for nxt in adj[todo.pop()]:
                        if nxt not in seen:seen.add(nxt);todo.append(nxt)
                self.assertEqual(seen,members,f'{preset} patch {patch}: disconnected authoring components')
    def test_radial_capture_is_one_continuous_open_spiral(self):
        out,_=self.generate('radial',broken=0)
        fs=[f for f in json.loads((out/'curves.json').read_text())['curves'] if not f['structural']]
        self.assertGreater(len(fs),200)
        for a,b in zip(fs,fs[1:]): self.assertEqual(a['node_b'],b['node_a'])
if __name__=='__main__':unittest.main()
