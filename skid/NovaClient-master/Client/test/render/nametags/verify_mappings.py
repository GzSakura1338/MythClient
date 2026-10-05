"""Check NameTags bindings against local Lunar, Forge and 26.3 class files; no Java source."""
import importlib.util
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('class_reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path.home() / '.lunarclient/offline/multiver/cache'
lunar = max([*cache.glob('*/bake.zip'), *cache.glob('*/*/bake.zip')], key=lambda p: p.stat().st_mtime)
forge = Path.home() / 'AppData/Roaming/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar'
modern = Path('D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar')
for filename, archive_path in [('Lunar189.inc', lunar), ('forge1201.inc', forge), ('minecraft263.inc', modern)]:
    source = (client / 'src/minecraft/compat/mappings' / filename).read_text(encoding='utf-8')
    section = re.split(r'#(?:if|elif) defined\(NOVA_NAMETAGS_MAPPING\)', source, maxsplit=1)[1].split('#elif', 1)[0]
    entries = re.findall(r'NOVA_NAMETAGS\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', section)
    classes = re.findall(r'NOVA_NAMETAGS_CLASS\(\w+, "([^"]+)"\)', section)
    assert len(entries) >= 7, (filename, "Empty or incomplete mapping section")
    with zipfile.ZipFile(archive_path) as archive:
        parsed = {}
        def read(owner):
            key = owner.replace('/', '.') if filename == 'Lunar189.inc' else owner.replace('.', '/') + '.class'
            return archive.read(key)
        def lookup(owner, name, descriptor):
            if owner not in parsed:
                try:
                    parsed[owner] = reader.read_class(read(owner))
                except KeyError:
                    return False
            parents, members = parsed[owner]
            return (name, descriptor) in members or any(lookup(p, name, descriptor) for p in parents)
        for owner in classes:
            read(owner)
        missing = [key for key, owner, name, descriptor in entries if not lookup(owner, name, descriptor)]
        assert not missing, (filename, missing)
    print(f'{filename}: {len(entries)} methods and {len(classes)} classes verified')
