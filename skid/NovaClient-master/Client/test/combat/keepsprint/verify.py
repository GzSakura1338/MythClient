"""Validate actual mapping members; generate a native oracle from Rise's existing listener."""
import importlib.util
import os
from pathlib import Path
import re
import sys
import zipfile
sys.dont_write_bytecode = True
client = Path(__file__).resolve().parents[3]
out = client / 'build-vs2026/test-keepsprint'
out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('reader',client/'test/player/bridgeassist/verify_mappings.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
cache = Path.home()/'.lunarclient/offline/multiver/cache'
lunar = Path(os.environ['NOVA_LUNAR189_BAKE']) if 'NOVA_LUNAR189_BAKE' in os.environ else max([*cache.glob('*/bake.zip'),*cache.glob('*/*/bake.zip')], key=lambda p:p.stat().st_mtime)
mc = Path(os.environ['APPDATA'])/'.minecraft'
jars = {'Lunar189.inc': lunar,
        'forge1201.inc':mc/'libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar',
        'minecraft263.inc':mc/'versions/26.3/26.3.jar'}
for filename,jar in jars.items():
    text=(client/'src/minecraft/compat/mappings'/filename).read_text(encoding='utf-8')
    section=re.split(r'#(?:if|elif) defined\(NOVA_KEEP_SPRINT_MAPPING\)',text,maxsplit=1)[1].split('#elif',1)[0]
    members=re.findall(r'\{"([^"]+)", "([^"]+)", "([^"]+)"\}',section)
    with zipfile.ZipFile(jar) as archive:
        parsed={}
        def read(owner):
            key=owner.replace('/','.') if filename=='Lunar189.inc' else owner.replace('.','/')+'.class'
            return archive.read(key)
        def lookup(owner,name,desc):
            if owner not in parsed:
                try: parsed[owner]=reader.read_class(read(owner))
                except KeyError: return False
            parents,values=parsed[owner]
            return (name,desc) in values or any(lookup(p,name,desc) for p in parents)
        for member in members:
            assert lookup(*member),('MISSING',filename,member)
        profile='lunar' if filename=='Lunar189.inc' else 'forge' if filename=='forge1201.inc' else '263'
        owners=['net/minecraft/entity/player/EntityPlayer','net/minecraft/client/multiplayer/PlayerControllerMP'] if profile=='lunar' else ['net/minecraft/world/entity/player/Player','net/minecraft/client/multiplayer/MultiPlayerGameMode']
        for i,owner in enumerate(owners): (out/f'{profile}-{i}.class').write_bytes(read(owner))
        print(f'PASS: {filename} {len(members)} JNI members in actual game classes')

rise=(client.parent/'skid/Rise-6.9.5-main/src/main/java/com/alan/clients/module/impl/ghost/KeepSprint.java').read_text()
body=rise.split('onHitSlowDown = var1 -> {',1)[1].split('\n    };',1)[0]
for old,new in {
    'aEg.thePlayer.onGround':'ground','aEg.thePlayer.hurtTime':'hurt',
    'this.onlyInAir.wo()':'settings.onlyAir','this.bufferAbuse.wo()':'settings.buffer',
    'this.maxBuffer.wo().intValue()':'settings.maximum','this.bufferDecrease.wo().doubleValue()':'settings.decrease',
    'this.slowDownVelocity.wo().doubleValue()':'settings.velocity','this.slowDownNormal.wo().doubleValue()':'settings.normal',
    'this.sprintSlowDownVelocity.wo()':'settings.velocitySprint','this.sprintSlowDownNormal.wo()':'settings.normalSprint',
    'this.combo':'combo','this.resetting':'resetting','Math.max':'std::max',
}.items(): body=body.replace(old,new)
body=re.sub(r'var1.setSlowDown\(([^;]+)\);',r'var1.slowdown = \1;',body)
body=re.sub(r'var1.setSprint\(([^;]+)\);',r'var1.sprint = \1;',body)
fixture=(client/'test/combat/keepsprint/StateTest.cpp').read_text()
(out/'state.cpp').write_text(fixture.replace('// @RISE_LISTENER@','void Listener(Result& var1) {'+body+'\n}'))
