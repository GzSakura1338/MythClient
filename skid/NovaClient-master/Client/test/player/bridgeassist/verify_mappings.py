"""Check the native bridge's named/SRG members directly in local class files (no Java tools)."""
import os
import re
import struct
import sys
import zipfile
from pathlib import Path


def read_class(data):
    offset = 8

    def u1():
        nonlocal offset
        value = data[offset]
        offset += 1
        return value

    def u2():
        nonlocal offset
        value = struct.unpack_from('>H', data, offset)[0]
        offset += 2
        return value

    def u4():
        nonlocal offset
        value = struct.unpack_from('>I', data, offset)[0]
        offset += 4
        return value

    pool = [None] * u2()
    index = 1
    while index < len(pool):
        tag = u1()
        if tag == 1:
            size = u2()
            pool[index] = data[offset:offset+size].decode('utf-8', errors='replace')
            offset += size
        elif tag in (7, 8, 16, 19, 20):
            pool[index] = u2()
        elif tag in (3, 4, 9, 10, 11, 12, 17, 18):
            offset += 4
        elif tag in (5, 6):
            offset += 8
            index += 1
        elif tag == 15:
            offset += 3
        else:
            raise ValueError(f'Unknown constant pool tag {tag}')
        index += 1
    u2()
    u2()
    superclass = u2()
    parents = [pool[pool[superclass]]] if superclass else []
    parents += [pool[pool[u2()]] for _ in range(u2())]
    members = set()
    for _ in range(2):
        for _ in range(u2()):
            u2()
            name, descriptor = pool[u2()], pool[u2()]
            members.add((name, descriptor))
            for _ in range(u2()):
                u2()
                size = u4()
                offset += size
    return parents, members


def verify(mapping, jars):
    archives = [zipfile.ZipFile(jar) for jar in jars]
    cache = {}

    def lookup(owner, name, descriptor):
        if owner not in cache:
            cache[owner] = None
            for archive in archives:
                try:
                    cache[owner] = read_class(archive.read(owner + '.class'))
                    break
                except KeyError:
                    pass
        entry = cache[owner]
        if entry is None:
            return False
        parents, members = entry
        return (name, descriptor) in members or any(lookup(p, name, descriptor) for p in parents)

    text = mapping.read_text(encoding='utf-8').split('#if defined(NOVA_BRIDGEASSIST_MAPPING)', 1)[1].split('#elif', 1)[0]
    entries = re.findall(r'NOVA_BRIDGE\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', text)
    missing = []
    for key, owner, name, descriptor in entries:
        if not lookup(owner.replace('.', '/'), name, descriptor):
            missing.append((key, owner, name, descriptor))
    for archive in archives:
        archive.close()
    for member in missing:
        print('MISSING', member)
    print(f'{mapping.name}: {len(entries)-len(missing)}/{len(entries)} members verified')
    return not missing


if __name__ == '__main__':
    client = Path(__file__).resolve().parents[3]
    minecraft = Path(os.environ['APPDATA']) / '.minecraft'
    forge = minecraft / 'libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar'
    modern = minecraft / 'versions/26.3/26.3.jar'
    results = [verify(client / 'src/minecraft/compat/mappings/forge1201.inc', [forge]),
               verify(client / 'src/minecraft/compat/mappings/minecraft263.inc', [modern])]
    sys.exit(0 if all(results) else 1)
