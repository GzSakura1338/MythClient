"""Read actual cached Lunar bytecode; generate only native C++ test fixtures."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
out = client / 'build-vs2026/test-lunar-incoming'
out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('reader', client / 'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path(os.environ['NOVA_LUNAR189_BAKE']) if 'NOVA_LUNAR189_BAKE' in os.environ else max((Path.home() / '.lunarclient/offline/multiver/cache').rglob('bake.zip'), key=lambda p: p.stat().st_mtime)
mapping = (client / 'src/minecraft/compat/mappings/Lunar189.inc').read_text()
classes = dict(re.findall(r'NOVA_CLASS\((\w+), "([^"]+)"\)', mapping))
ids = {'TaskPacket', 'TaskListener', 'IsClientThread', 'ScheduleTask', 'PacketTaskCtor'}
found = 0
with zipfile.ZipFile(cache) as jar:
    for id_, owner, name, desc in re.findall(r'NOVA_(?:FIELD|METHOD)\((\w+), (\w+), "([^"]+)", "([^"]+)"\)', mapping):
        if id_ not in ids:
            continue
        _, members = reader.read_class(jar.read(classes[owner]))
        assert (name, desc) in members, (id_, name, desc)
        found += 1
    (out / 'PacketTask.class').write_bytes(jar.read('net.minecraft.network.PacketThreadUtil$1'))
assert found == len(ids)
source = (client / 'src/minecraft/compat/lunar189/LunarRuntime.cpp').read_text()
body = source.split('void ProcessClientPacket(', 1)[1].split('void JNICALL BlockVanillaUseHook(', 1)[0]
prefix = (Path(__file__).with_name('Test.prefix.cpp')).read_text()
suffix = (Path(__file__).with_name('Test.suffix.cpp')).read_text()
(out / 'runtime.cpp').write_text(prefix + '\nvoid ProcessClientPacket(' + body + '\n' + suffix)
assert '{"clientPacketTask", "(Ljava/lang/Object;)Z", reinterpret_cast<void*>(&ClientPacketTaskHook)}' in source
print('PASS: 5 scheduler/task mappings in actual Lunar classes; native hook registration; actual runtime extracted')
