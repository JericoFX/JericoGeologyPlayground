"""Small content-addressed evidence store; no data in earlier runs is replaced."""
import hashlib
import json
from pathlib import Path


def freeze_file(path,label):
    path=Path(path)
    data=path.read_bytes();digest=hashlib.sha256(data).hexdigest()
    target=path.parent/(label+'-'+digest[:12]+path.suffix)
    if target.exists():
        if target.read_bytes()!=data:raise RuntimeError('Existing content-addressed evidence is corrupt: '+str(target))
        if path!=target:path.unlink()
    else:
        # Exclusive creation also catches a same-name concurrent writer.
        with target.open('xb') as stream:stream.write(data)
        if path!=target:path.unlink()
    return {'file':str(target),'sha256':digest}


def freeze_manifest(records,directory):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=True)
    data=(json.dumps(records,indent=2,sort_keys=True,allow_nan=False)+'\n').encode()
    target=directory/('render-manifest-'+hashlib.sha256(data).hexdigest()[:12]+'.json')
    if target.exists():
        if target.read_bytes()!=data:raise RuntimeError('Manifest hash collision')
    else:
        with target.open('xb') as stream:stream.write(data)
    return target
