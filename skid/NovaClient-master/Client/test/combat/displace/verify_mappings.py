"""Read original Lunar class files to validate Displace bindings, without Java output."""
import importlib.util
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
jar = max([*cache.glob('*/bake.zip'), *cache.glob('*/*/bake.zip')], key=lambda p:p.stat().st_mtime)
text = (client / 'src/minecraft/compat/mappings/Lunar189.inc').read_text(encoding='utf-8')
with zipfile.ZipFile(jar) as archive:
    parsed = {}
    def lookup(owner,name,desc):
        if owner not in parsed:
            try: parsed[owner] = reader.read_class(archive.read(owner.replace('/','.')))
            except KeyError: return False
        parents,members = parsed[owner]
        return (name,desc) in members or any(lookup(p,name,desc) for p in parents)
    entries = re.findall(r'NOVA_DISPLACE\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', text)
    assert entries
    for key,owner,name,desc in entries:
        assert lookup(owner,name,desc),(key,owner,name,desc)
    for owner in re.findall(r'NOVA_DISPLACE_CLASS\(\w+, "([^"]+)"\)',text):
        assert archive.read(owner)
    assert lookup('net.minecraft.client.Minecraft','runTick','()V')
    assert lookup('net.minecraft.client.renderer.EntityRenderer','getMouseOver','(F)V')
    output = client / 'build-vs2026/test-displace'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'Minecraft.class').write_bytes(archive.read('net.minecraft.client.Minecraft'))
print(f'PASS: {len(entries)} Displace JNI bindings and rotation hook members in original Lunar classes')
