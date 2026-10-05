"""Validate Nova's native source ownership and rooted includes without building."""
from collections import Counter
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[1] / 'Client'
source = client / 'src'
vendor = source / 'third_party'
errors = []

categories = ('combat', 'misc', 'movement', 'player', 'render')
for category in categories:
    directory = source / 'minecraft' / 'modules' / category
    for path in directory.iterdir():
        if path.is_file():
            errors.append(f'Module has no feature directory: {path.relative_to(client)}')

# Translation units are explicit: each native implementation must belong to
# exactly one source manifest. Headers are globbed only for IDE navigation.
manifests = [client / 'CMakeLists.txt', *(source / name / 'CMakeLists.txt'
             for name in ('minecraft', 'minecraft/compat', 'minecraft/java', 'minecraft/modules', 'cs2', 'ui', 'smtc'))]
listed = []
for manifest in manifests:
    text = manifest.read_text(encoding='utf-8-sig')
    for line in text.splitlines():
        match = re.fullmatch(r'\s*([\w/.-]+\.(?:cpp|c))\)?\s*', line)
        if match:
            path = (manifest.parent / match[1]).resolve()
            if path.is_relative_to(source.resolve()) and not path.is_relative_to(vendor.resolve()):
                listed.append(path)
                if not path.is_file():
                    errors.append(f'Missing CMake source: {path.relative_to(client)}')
actual = {p.resolve() for p in source.rglob('*')
          if p.suffix in ('.cpp', '.c') and not p.is_relative_to(vendor)}
for path in sorted(actual - set(listed)):
    errors.append(f'Unregistered implementation: {path.relative_to(client)}')
for path, count in Counter(listed).items():
    if count != 1:
        errors.append(f'Duplicate CMake source ({count}): {path.relative_to(client)}')

include_roots = [source, client / 'include', client,
                 vendor / 'fatalwidgets/include',
                 vendor / 'fatalwidgets/imgui',
                 vendor / 'vulkan/include',
                 vendor / 'minhook/include', vendor / 'minhook/src']
files = [p for base in (source, client / 'include') for p in base.rglob('*')
         if p.suffix in ('.cpp', '.c', '.h', '.inc') and not p.is_relative_to(vendor)]
includes = 0
for path in files:
    for name in re.findall(r'^\s*#\s*include\s+"([^"]+)"', path.read_text(encoding='utf-8-sig'), re.M):
        includes += 1
        if not any((base / name).is_file() for base in (path.parent, *include_roots)):
            errors.append(f'Unresolved include {name}: {path.relative_to(client)}')

# The presentation target must stay independent even through transitive headers.
# Host entry points and all game-only APIs stay inside the game directory.
shared_entries = [p for p in files if any(p.is_relative_to(source / name)
                  for name in ('ui', 'contracts', 'config', 'smtc', 'bind', 'utils'))]
forbidden = [source / name for name in ('minecraft', 'cs2')]
for name in ('module', 'compat', 'java', 'hook', 'eventbus', 'util', 'setting', 'games'):
    if (source / name).exists() and any((source / name).rglob('*')):
        errors.append(f'Legacy source directory is not empty: src/{name}')
forbidden_resolved = [p.resolve() for p in forbidden]
visited = set()
pending = [(p, p) for p in shared_entries]
while pending:
    path, entry = pending.pop()
    path = path.resolve()
    if path in visited:
        continue
    visited.add(path)
    if any(path.is_relative_to(p) for p in forbidden_resolved):
        errors.append(f'Shared UI includes host code: {entry.relative_to(client)} -> {path.relative_to(client)}')
        continue
    if path.is_relative_to(vendor.resolve()):
        continue
    for name in re.findall(r'^\s*#\s*include\s+"([^"]+)"', path.read_text(encoding='utf-8-sig'), re.M):
        resolved = next(((root / name).resolve() for root in (path.parent, *include_roots)
                         if (root / name).is_file()), None)
        if resolved:
            pending.append((resolved, entry))

for path in (client / 'docs').glob('*.md'):
    for target in re.findall(r'\]\(([^)]+)\)', path.read_text(encoding='utf-8-sig')):
        if re.match(r'^(?:[\w+.-]+:|#)', target):
            continue
        target = target.split('#', 1)[0]
        if target and not (path.parent / target).exists():
            errors.append(f'Broken document link {target}: {path.relative_to(client)}')

if errors:
    print('\n'.join(errors))
    sys.exit(1)
print(f'PASS: {len(actual)} native implementations registered once, {includes} includes resolved, '
      'shared UI dependency boundary, module ownership and document links valid')
