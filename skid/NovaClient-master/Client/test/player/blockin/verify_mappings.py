from pathlib import Path
import importlib.util, os, re, sys, zipfile
sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('reader', client/'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec); spec.loader.exec_module(reader)
mapping = (client/'src/minecraft/compat/mappings/Lunar189.inc').read_text(encoding='utf-8')
entries = re.findall(r'NOVA_BLOCKIN\((\w+), "([^"]+)", "([^"]+)", "([^"]+)"\)', mapping)
root = Path.home()/'.lunarclient/offline/multiver/cache'
archive = Path(os.environ['NOVA_LUNAR189_BAKE']) if 'NOVA_LUNAR189_BAKE' in os.environ else max(root.glob('**/bake.zip'), key=lambda p:p.stat().st_mtime)
cache = {}
with zipfile.ZipFile(archive) as jar:
    def lookup(owner,name,desc):
        owner=owner.replace('.','/')
        if owner == 'java/lang/Object' and name == 'toString': return True
        if owner not in cache:
            try: cache[owner]=reader.read_class(jar.read(owner.replace('/','.')))
            except KeyError: return False
        parents,members=cache[owner]
        return (name,desc) in members or any(lookup(p,name,desc) for p in parents)
    missing=[key for key,owner,name,desc in entries if not lookup(owner,name,desc)]
    assert entries and not missing, missing
print(f'BlockIn: {len(entries)} native mappings verified in Lunar {archive.parent.name}')
