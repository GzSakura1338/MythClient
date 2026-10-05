"""Audit legacy JNI symbols against an explicit Lunar bake, without changing code."""
import importlib.util
import json
import re
import sys
import zipfile
from pathlib import Path

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
archive = zipfile.ZipFile(sys.argv[1])
mapping = sys.argv[2] if len(sys.argv) > 2 else 'Lunar189.inc'
source = (client / 'src/minecraft/compat/mappings' / mapping).read_text(encoding='utf-8')
section = '#elif defined(NOVA_LUNAR1710_RUNTIME)' if mapping == 'Lunar1710.inc' else '#elif defined(NOVA_CLASS)'
runtime = source.split(section, 1)[1]
classes = dict(re.findall(r'NOVA_CLASS\((\w+), "([^"]+)"\)', runtime))
cache = {}
def read(owner):
    owner = owner.replace('/', '.')
    if owner not in cache:
        try:
            cache[owner] = reader.read_class(archive.read(owner))
        except KeyError:
            cache[owner] = ([], set())
    return cache[owner]

def lookup(owner, name, desc):
    if owner.startswith(('java.', 'minecraft/java/')):
        return (owner.replace('/', '.'), name, desc) in {
            ('java.lang.Object', 'toString', '()Ljava/lang/String;'),
            ('java.lang.Enum', 'ordinal', '()I'),
            ('java.util.List', 'size', '()I'),
            ('java.util.List', 'get', '(I)Ljava/lang/Object;'),
            ('java.nio.FloatBuffer', 'get', '(I)F'),
        }
    parents, members = read(owner)
    return (name, desc) in members or any(lookup(p, name, desc) for p in parents)

missing = []
for kind, key, owner, name, desc in re.findall(r'NOVA_(METHOD|FIELD|STATIC_METHOD|STATIC_FIELD)\((\w+), (\w+), "([^"]+)", "([^"]+)"\)', runtime):
    if not lookup(classes[owner], name, desc):
        missing.append(dict(kind=kind, key=key, owner=classes[owner], name=name, descriptor=desc))
print(json.dumps(dict(missing_classes={k:v for k,v in classes.items() if not v.startswith('java.') and not read(v)[1]}, missing_members=missing), indent=2))
if mapping == 'Lunar1710.inc':
    aliases = {}
    for oc,on,od,nc,nn,nd in re.findall(r'NOVA_LUNAR1710_ALIAS\("([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)"\)', source):
        aliases[(oc,on,od)] = (nc,nn,nd)
        if not lookup(nc,nn,nd):
            missing.append((nc,nn,nd))
            print('MISSING alias',nc,nn,nd)
    for key, owner, name, desc in re.findall(r'NOVA_LUNAR1710_MEMBER\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', source):
        if not lookup(owner, name, desc):
            missing.append(key)
            print('MISSING member', key, owner, name, desc)
    if '--supplementary' in sys.argv:
        legacy = (client / 'src/minecraft/compat/mappings/Lunar189.inc').read_text(encoding='utf-8').split('#elif defined(NOVA_CLASS)',1)[0]
        for key,owner,name,desc in re.findall(r'NOVA_\w+\(([^,]+), "([^"]+)", "([^"]+)", "([^"]+)"\)',legacy):
            symbol=aliases.get((owner,name,desc),(owner,name,desc))
            if not owner.startswith('net.minecraft.') or lookup(*symbol): continue
            print('SUPPLEMENTARY',key,owner,name,desc)
    absent=[v for v in classes.values() if not v.startswith('java.') and not read(v)[1]]
    sys.exit(1 if missing or absent else 0)
