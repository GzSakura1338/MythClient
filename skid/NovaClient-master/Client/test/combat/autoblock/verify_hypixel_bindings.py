"""Read cached Lunar class files to validate Hypixel JNI bindings; no JVM/Java source."""
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
text = mapping.split('// Leader Hypixel AutoBlock immediate packet / local-use operations.', 1)[1].split('#else')[0]
members = re.findall(r'NOVA_(?:STATIC_)?(?:FIELD|METHOD)\((\w+), (\w+), "([^"]+)", "([^"]+)"\)', text)
cache_root = Path.home() / '.lunarclient/offline/multiver/cache'
archives = sorted(cache_root.glob('*/bake.zip')) + sorted(cache_root.glob('*/*/bake.zip'))
archive_path = max(archives, key=lambda p: p.stat().st_mtime)
with zipfile.ZipFile(archive_path) as archive:
    parsed = {}
    def class_bytes(owner):
        # Lunar bake.zip uses dotted class names without the .class suffix.
        return archive.read(owner.replace('/', '.'))
    def lookup(owner, name, descriptor):
        if owner not in parsed:
            try:
                parsed[owner] = reader.read_class(class_bytes(owner))
            except KeyError:
                return False
        parents, values = parsed[owner]
        return (name, descriptor) in values or any(lookup(parent, name, descriptor) for parent in parents)
    missing = [key for key, owner, name, descriptor in members if not lookup(classes[owner], name, descriptor)]
    print(f'Lunar {archive_path.parent.name}: {len(members)-len(missing)}/{len(members)} Hypixel bindings verified')
    for key in missing:
        print('MISSING', key)
    # Native bytecode test input. Read the current cached build, not stale
    # hand-authored or decompiled source; leave Recaf's Leader workspace alone.
    out = client / 'build-vs2026/test-hypixel/classes'
    out.mkdir(parents=True, exist_ok=True)
    for name in ['net/minecraft/client/Minecraft', 'net/minecraft/client/entity/EntityPlayerSP',
                 'net/minecraft/entity/player/EntityPlayer']:
        (out / (name.replace('/', '_') + '.class')).write_bytes(class_bytes(name))
keyboard = re.search(r'NOVA_STATIC_METHOD\(KeyboardKey, Keyboard, "([^"]+)", "([^"]+)"\)', mapping)
lwjgl_root = Path(os.environ['APPDATA']) / '.minecraft/libraries/org/lwjgl/lwjgl/lwjgl'
lwjgl_jar = next(lwjgl_root.glob('*/lwjgl-*.jar'))
with zipfile.ZipFile(lwjgl_jar) as archive:
    _, members = reader.read_class(archive.read(classes['Keyboard'].replace('.', '/') + '.class'))
    if keyboard.groups() not in members:
        missing.append('KeyboardKey')
    else:
        print('LWJGL physical use-key binding verified')
sys.exit(bool(missing))
