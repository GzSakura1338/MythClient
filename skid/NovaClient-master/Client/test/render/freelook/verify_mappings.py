"""Verify JNI mappings and extract original hook targets; never writes Java."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path.home() / '.lunarclient/offline/multiver/cache'
lunar = max([*cache.glob('*/bake.zip'), *cache.glob('*/*/bake.zip')], key=lambda p: p.stat().st_mtime)
profiles = {
    'lunar': (lunar, 'Lunar189.inc', [
        'net.minecraft.client.renderer.EntityRenderer', 'net.minecraft.client.renderer.RenderGlobal',
        'net.minecraft.client.renderer.ActiveRenderInfo', 'net.minecraft.client.renderer.entity.RenderManager']),
    'forge': (Path(os.environ['APPDATA']) / '.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar',
              'forge1201.inc', ['net.minecraft.client.Camera', 'net.minecraft.client.MouseHandler']),
    '263': (Path('D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar'), 'minecraft263.inc',
            ['net.minecraft.client.Camera', 'net.minecraft.client.MouseHandler'])
}
for profile, (jar, mapping, targets) in profiles.items():
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
        entries = re.findall(r'NOVA_FREELOOK\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', text)
        assert entries
        for key, owner, name, desc in entries:
            assert lookup(owner, name, desc), (profile, key, owner, name, desc)
        for name in targets:
            path = client / 'build-vs2026/test-freelook/classes' / profile / (name.replace('.', '/') + '.class')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(get(name))
        print(f'PASS: {profile} {len(entries)} FreeLook JNI members in original classes; {len(targets)} targets extracted')

# Every new edit must have an identically typed native binding in that profile.
for profile, (_, mapping, _) in profiles.items():
    text = (client / 'src/minecraft/compat/mappings' / mapping).read_text(encoding='utf-8')
    source = (client / 'src/minecraft/compat/lunar189/LunarRuntime.cpp').read_text(encoding='utf-8') if profile == 'lunar' else text
    bindings = set(re.findall(r'\{"(freeLook\w+)", "([^"]+)", reinterpret_cast', source))
    edits = set(re.findall(r'"(freeLook\w+)", "([^"]+)"', text))
    assert edits and edits <= bindings, (profile, edits - bindings)
print('PASS: all FreeLook edits have matching native hook signatures')
