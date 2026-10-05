"""Extract local original class bytes for read-only native hook audits."""
from pathlib import Path
import os
import zipfile

client = Path(__file__).resolve().parents[3]
out = client / 'build-vs2026/test-slotspoof/classes'
profiles = {
    'forge': (Path(os.environ['APPDATA']) / '.minecraft/libraries/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar', [
        'net/minecraft/client/renderer/GameRenderer',
        'net/minecraft/world/entity/player/Inventory', 'net/minecraft/world/entity/player/Player',
        'net/minecraft/client/renderer/ItemInHandRenderer', 'net/minecraft/client/multiplayer/MultiPlayerGameMode']),
    '263': (Path('D:/AI/VibeCoding/shader/skid/minecraft-26.3-client.jar'), [
        'net/minecraft/client/Camera',
        'net/minecraft/world/entity/player/Inventory', 'net/minecraft/client/player/FirstPersonHandsAndItems',
        'net/minecraft/client/multiplayer/MultiPlayerGameMode'])
}
for profile, (jar, classes) in profiles.items():
    with zipfile.ZipFile(jar) as archive:
        for name in classes:
            path = out / profile / (name + '.class')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(archive.read(name + '.class'))
print('Extracted original Forge and 26.3 slot/render/controller classes')
