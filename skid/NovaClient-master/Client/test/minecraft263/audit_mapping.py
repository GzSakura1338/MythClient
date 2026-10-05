"""Verify catalog descriptors against the same 26.3 JAR inspected in Recaf.

Does not execute Minecraft or modify the JAR / Recaf workspace.
"""
import argparse
from pathlib import Path
import re
import struct
import zipfile


def members(data):
    pos = 8
    def u2():
        nonlocal pos
        v = struct.unpack_from('>H', data, pos)[0]
        pos += 2
        return v
    count = u2()
    pool = [None] * count
    i = 1
    while i < count:
        tag = data[pos]
        pos += 1
        if tag == 1:
            length = u2()
            pool[i] = data[pos:pos + length].decode('utf-8', errors='replace')
            pos += length
        elif tag in (3, 4, 9, 10, 11, 12, 17, 18): pos += 4
        elif tag in (5, 6): pos += 8; i += 1
        elif tag in (7, 8, 16, 19, 20): pos += 2
        elif tag == 15: pos += 3
        else: raise ValueError(f'Unknown constant tag {tag}')
        i += 1
    pos += 6
    interfaces = u2()
    pos += interfaces * 2
    result = {}
    for kind in ('FIELD', 'METHOD'):
        for _ in range(u2()):
            flags, name, desc = u2(), u2(), u2()
            result[kind, pool[name], pool[desc]] = bool(flags & 8)
            for _ in range(u2()):
                pos += 2
                length = struct.unpack_from('>I', data, pos)[0]
                pos += 4 + length
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('jar', type=Path)
    parser.add_argument('--extract-hooks', type=Path)
    args = parser.parse_args()
    catalog = Path(__file__).resolve().parents[2] / 'src/minecraft/compat/mappings/minecraft263.inc'
    source = catalog.read_text(encoding='utf-8')
    classes = dict(re.findall(r'^NOVA_CLASS\((\w+), "([^"]+)"\)', source, re.M))
    declarations = re.findall(r'^NOVA_(STATIC_)?(FIELD|METHOD)\((\w+), (\w+), "([^"]+)", "([^"]+)"\)', source, re.M)
    failures, checked, external = [], 0, 0
    with zipfile.ZipFile(args.jar) as jar:
        cache = {}
        paths = set(jar.namelist())
        for ident, name in classes.items():
            if not name.startswith(('java.', 'org.lwjgl.', 'org.joml.')) and name.replace('.', '/') + '.class' not in paths:
                failures.append(f'{ident}: missing class {name}')
        for static, kind, ident, owner, name, desc in declarations:
            path = classes[owner].replace('.', '/') + '.class'
            if path not in paths:
                if classes[owner].startswith(('java.', 'org.lwjgl.', 'org.joml.')):
                    external += 1
                    continue
                failures.append(f'{ident}: missing class {path}')
                continue
            if path not in cache: cache[path] = members(jar.read(path))
            found = cache[path].get((kind, name, desc))
            if found is None or found != bool(static):
                failures.append(f'{ident}: wrong name/descriptor/static flag ({owner}.{name}{desc})')
            checked += 1
        if args.extract_hooks:
            edit_section = source.split('#elif defined(NOVA_MC263_BINDINGS)')[0]
            for name in re.findall(r'^    \{"(net/[^\"]+)",', edit_section, re.M):
                target = args.extract_hooks / (name + '.class')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(jar.read(name + '.class'))
    for failure in failures: print(failure)
    print(f'{checked} game members checked; {external} external members; {len(failures)} failures')
    raise SystemExit(bool(failures))


if __name__ == '__main__': main()
