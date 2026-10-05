"""Read-only validation against the user's actual three game profiles."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile
sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('reader', client/'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path.home()/'.lunarclient/offline/multiver/cache'
lunar = max([*cache.glob('*/bake.zip'), *cache.glob('*/*/bake.zip')], key=lambda p:p.stat().st_mtime)
mc = Path(os.environ['APPDATA'])/'.minecraft'
out = client/'build-vs2026/test-breaker'
out.mkdir(exist_ok=True)
profiles = [('Lunar189', lunar, 'lunar'),
            ('forge1201', mc/'libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar', 'forge'),
            ('minecraft263', mc/'versions/26.3/26.3.jar', '263')]
for filename, jar, profile in profiles:
    with zipfile.ZipFile(jar) as archive:
        parsed = {}
        def read(owner):
            key = owner.replace('/', '.') if profile == 'lunar' else owner.replace('.', '/')+'.class'
            return archive.read(key)
        def lookup(owner, name, desc):
            if owner not in parsed:
                try: parsed[owner] = reader.read_class(read(owner))
                except KeyError: return False
            parents, members = parsed[owner]
            return (name, desc) in members or any(lookup(p, name, desc) for p in parents)
        text = (client/'src/minecraft/compat/mappings'/f'{filename}.inc').read_text(encoding='utf-8')
        members = re.findall(r'NOVA_BREAKER\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', text)
        assert len(members) == 34
        for key, owner, name, desc in members:
            assert lookup(owner, name, desc), (profile, key, owner, name, desc)
        (out/f'{profile}.class').write_bytes(read('net.minecraft.client.Minecraft'))
        print(f'PASS: {profile}: {len(members)} Breaker members in actual game classes')
