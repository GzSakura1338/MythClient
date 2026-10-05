"""Validate AutoPot JNI members against original game class files; no Java output."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
prefix = sys.argv[1] if len(sys.argv) > 1 else 'NOVA_AUTOPOT'
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path.home() / '.lunarclient/offline/multiver/cache'
lunar = max([*cache.glob('*/bake.zip'), *cache.glob('*/*/bake.zip')], key=lambda p: p.stat().st_mtime)
profiles = {
    'lunar': (lunar, 'Lunar189.inc'),
    'forge': (Path(os.environ['APPDATA']) / '.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar', 'forge1201.inc'),
    '263': (Path('D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar'), 'minecraft263.inc'),
}
for profile, (jar, mapping) in profiles.items():
    with zipfile.ZipFile(jar) as archive:
        parsed = {}
        def get(owner):
            return archive.read(owner.replace('/', '.') if profile == 'lunar' else owner.replace('.', '/') + '.class')
        def lookup(owner, name, desc):
            if owner not in parsed:
                try:
                    parsed[owner] = reader.read_class(get(owner))
                except KeyError:
                    return False
            parents, members = parsed[owner]
            return (name, desc) in members or any(lookup(p, name, desc) for p in parents)
        text = (client / 'src/minecraft/compat/mappings' / mapping).read_text(encoding='utf-8')
        entries = re.findall(re.escape(prefix) + r'\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', text)
        assert entries
        for key, owner, name, desc in entries:
            assert lookup(owner, name, desc), (profile, key, owner, name, desc)
        for owner in re.findall(re.escape(prefix) + r'_CLASS\("([^"]+)"\)', text):
            assert get(owner)
        print(f'PASS: {profile} {len(entries)} {prefix} JNI members/classes in original game')
