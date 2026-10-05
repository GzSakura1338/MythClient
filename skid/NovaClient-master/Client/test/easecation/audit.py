"""Read-only JNI catalog audit against the named NeoForge client; no Java source."""
import importlib.util
import json
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('reader', root / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
catalog = (root / 'src/minecraft/compat/mappings/minecraft263.inc').read_text()
ec = (root / 'src/minecraft/compat/mappings/easecation.inc').read_text()
with zipfile.ZipFile(sys.argv[1]) as archive:
    if 'version.json' in archive.namelist():
        version = json.loads(archive.read('version.json'))['id']
        if len(sys.argv) > 3 and sys.argv[3] != version:
            raise ValueError('JAR version does not match requested version')
    elif len(sys.argv) > 3:
        version = sys.argv[3]
    elif re.fullmatch(r'neoforge-21\.8\.\d+-client\.jar', Path(sys.argv[1]).name):
        version = '1.21.8'
    else:
        raise ValueError('JAR has no version.json; specify its verified game version')
if version not in {'1.21.8', '1.21.10'}:
    raise ValueError(f'Unsupported NeoForge audit version: {version}')
modern = version == '1.21.10'
prefix = 'NOVA_NEOFORGE12110' if modern else 'NOVA_EASECATION'
label = 'NeoForge' if modern else 'EaseCation'
if modern:
    ec = (root / 'src/minecraft/compat/mappings/neoforge12110.inc').read_text()
classes = dict(re.findall(r'NOVA_CLASS\((\w+), "([^"]+)"\)', catalog))
types = dict(re.findall(prefix + r'_TYPE\("([^"]+)",\s*"([^"]+)"\)', ec))
aliases = {tuple(m[:3]): tuple(m[3:]) for m in re.findall(prefix + r'_ALIAS\("([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)', ec)}
used = set()
for source in (root / 'src').rglob('*.cpp'):
    used.update(re.findall(r'\b\w+\b', source.read_text(encoding='utf-8-sig', errors='replace')))

# These operations have explicit EaseCation ABI/semantic implementations instead
# of calling the 26.3 member. Keep this list narrow so new mismatches fail loudly.
adapted = set('''RenderSystem_Field_BACKEND Minecraft_Field_windowSurface Gui_screen
Minecraft_pick GameRenderer_gameRenderState ClientAsset_Texture_texturePath
ClientboundSetEntityMotionPacket_movement Entity_lerpMotion LivingEntity_swing
PlayerSkin_body ItemStack_getAttackAnimation ItemAttributeModifiers_compute
GameRenderState_Field_levelRenderState LevelRenderState_Field_cameraRenderState
CameraRenderState_Field_pos CameraRenderState_Field_xRot CameraRenderState_Field_yRot
CameraRenderState_Field_initialized CameraRenderState_Field_projectionMatrix
CameraRenderState_Field_viewRotationMatrix Level_getRespawnData RespawnData_pos
RespawnData_dimension ServerboundPunchPacket_Field_INSTANCE NativeImage_isClosed'''.split())
if modern:
    adapted.difference_update('''ClientAsset_Texture_texturePath
ClientboundSetEntityMotionPacket_movement Entity_lerpMotion PlayerSkin_body
Level_getRespawnData RespawnData_pos RespawnData_dimension'''.split())
entries = []
for ident, owner, name, desc in re.findall(r'NOVA_(?:STATIC_)?(?:FIELD|METHOD)\((\w+), (\w+), "([^"]+)", "([^"]+)"\)', catalog):
    if ident in used and ident not in adapted and not classes[owner].startswith('com.mojang.renderpearl.'):
        entries.append((ident, classes[owner], name, desc))
entries += re.findall(prefix + r'_MEMBER\((\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)', ec)
for section in re.split(r'#(?:if|elif) defined\(', catalog.split('#elif defined(NOVA_MC263_EDITS)')[0]):
    category = section.split(')')[0]
    for ident, owner, name, desc in re.findall(r'NOVA_\w+\("?(\w+)"?, "([\w.$]+)", "([^"]+)", "([^"]+)"\)', section):
        if not desc.startswith(('(', 'L', '[', 'I', 'J', 'F', 'D', 'Z', 'B', 'S')): continue
        if category == 'NOVA_BACKTRACK_MAPPING' and ident in {'Path','Delta','delta','decode','end','syncPosition'}: continue
        if ident == 'physicalKey' or (ident == 'invertX' and not modern): continue
        if category == 'NOVA_TEAMS_MAPPING' and ident in {'Format','Color','Value'}: continue
        entries.append((category + ':' + ident, owner, name, desc))
entries += re.findall(r'NOVA_BT\("(\w+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)', ec)
entries += re.findall(r'NOVA_TEAM\((\w+),\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)"\)', ec)

with zipfile.ZipFile(sys.argv[1]) as archive:
    cache = {}
    def lookup(owner, name, desc):
        if owner.startswith(('java.', 'org.')): return True
        if owner not in cache:
            try: cache[owner] = reader.read_class(archive.read(owner.replace('.', '/') + '.class'))
            except KeyError: return False
        parents, members = cache[owner]
        return (name, desc) in members or any(lookup(p, name, desc) for p in parents if p != 'java.lang.Object')
    failures = []
    for ident, owner, name, desc in entries:
        owner, name, desc = aliases.get((owner, name, desc), (owner, name, desc))
        owner = types.get(owner, owner)
        for old, new in types.items(): desc = desc.replace('L'+old.replace('.', '/')+';', 'L'+new.replace('.', '/')+';')
        if not lookup(owner, name, desc): failures.append(f'{ident}: {owner}.{name}{desc}')
    if len(sys.argv) > 2:
        out = Path(sys.argv[2])
        table = 'neoforge12110Edits' if modern else 'easecationEdits'
        names = re.findall(r'^    \{"(net/[^\"]+)",', ec.split(f'constexpr ClassEdits {table}[]'+'{')[1], re.M)
        names.append('net/minecraft/client/gui/screens/ConnectScreen$1')
        for name in names:
            target = out / (name + '.class')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(archive.read(name + '.class'))
        start = catalog.index('#elif defined(NOVA_MC263_BINDINGS)')
        end = catalog.index('#elif', start + 6)
        bindings = catalog[start:end]
        bindings += (root / 'src/minecraft/compat/minecraft263/Minecraft263Hooks.cpp').read_text()
        bindings += (root / 'src/minecraft/proxy/ProxyHooks.cpp').read_text()
        hooks = re.findall(r'\{\s*"(\w+)"\s*,\s*"(\([^"]+)"\s*,\s*reinterpret_cast<void\*>', bindings)
        if len(hooks) < 30:
            raise ValueError('Missing native hook bindings')
        (out / 'native-hooks.inc').write_text('\n'.join(f'expected.emplace("{name}","{desc}");' for name, desc in hooks))
    print('\n'.join(failures))
    print(f'{label} {version}: {len(entries)} JNI bindings checked, {len(failures)} failures')
    sys.exit(bool(failures))
