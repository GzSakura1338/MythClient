"""Check new Slinky JNI names against actual Lunar bytes, without loading Java."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('class_reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
mapping = (client / 'src/minecraft/compat/mappings/Lunar189.inc').read_text(encoding='utf-8')
classes = dict(re.findall(r'NOVA_CLASS\((\w+), "([^"]+)"\)', mapping))
members = re.findall(r'NOVA_(?:STATIC_)?(?:FIELD|METHOD)\((BlockRegistry|RegistryName|ResourceString), (\w+), "([^"]+)", "([^"]+)"\)', mapping)
assert len(members) == 3
cache = Path.home() / '.lunarclient/offline/multiver/cache'
archives = list(cache.glob('*/bake.zip')) + list(cache.glob('*/*/bake.zip'))
archive_path = Path(os.environ['NOVA_LUNAR189_BAKE']) if 'NOVA_LUNAR189_BAKE' in os.environ else max(archives, key=lambda p: p.stat().st_mtime)
with zipfile.ZipFile(archive_path) as archive:
    parsed = {}
    def lookup(owner, name, descriptor):
        if owner not in parsed:
            try:
                parsed[owner] = reader.read_class(archive.read(owner.replace('/', '.')))
            except KeyError:
                return False
        parents, values = parsed[owner]
        return (name, descriptor) in values or any(lookup(parent, name, descriptor) for parent in parents)
    for key, owner, name, descriptor in members:
        assert lookup(classes[owner], name, descriptor), key
        assert mapping.index(f'NOVA_CLASS({owner},') < mapping.index(f'({key}, {owner},'), key
    for key in ['WindowClickPacket','ClosePacket','LoginStartPacket','HandshakePacket']:
        assert archive.read(classes[key])[:4] == b'\xca\xfe\xba\xbe', key
print('PASS: Slinky registry JNI descriptors, initialization order and packet classes in', archive_path)
