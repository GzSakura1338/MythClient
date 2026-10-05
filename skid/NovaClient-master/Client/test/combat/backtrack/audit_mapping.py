from pathlib import Path
import importlib.util, re, zipfile, sys
sys.dont_write_bytecode = True
spec=importlib.util.spec_from_file_location('audit',Path(__file__).parents[2]/'minecraft263/audit_mapping.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
root=Path(__file__).parents[3]
pairs=[('minecraft263.inc',Path('D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar')),('forge1201.inc',Path.home()/'AppData/Roaming/.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar')]
errors=[]
for filename,path in pairs:
 with zipfile.ZipFile(path) as jar:
  rows=re.findall(r'NOVA_BT\("([^"]+)", "([^"]+)", "([^"]*)", "([^"]*)"\)',(root/'src/minecraft/compat/mappings'/filename).read_text(encoding='utf-8-sig'))
  for key,owner,name,desc in rows:
   try: data=jar.read(owner.replace('.','/')+'.class')
   except KeyError:errors.append(f'{filename} missing class {owner}');continue
   if name and (('METHOD' if desc.startswith('(') else 'FIELD'),name,desc) not in audit.members(data):errors.append(f'{filename} missing {key}: {owner}.{name} {desc}')
  print(filename,len(rows),'bindings checked')
  if filename == 'forge1201.inc':
   target = root / 'build-vs2026/test-backtrack/forge-controller.class'
   target.parent.mkdir(parents=True, exist_ok=True)
   target.write_bytes(jar.read('net/minecraft/client/multiplayer/MultiPlayerGameMode.class'))
print('\n'.join(errors) if errors else 'Backtrack mappings: 0 failures')
sys.exit(bool(errors))
