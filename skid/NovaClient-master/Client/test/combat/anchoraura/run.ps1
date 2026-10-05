$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-anchoraura"
New-Item -ItemType Directory -Force $out | Out-Null
$fixture=Get-Content "$PSScriptRoot/../legitcrystalaura/RuntimeTest.cpp" -Raw
$prefix=$fixture.Substring(0,$fixture.IndexOf('// @PRODUCTION_MODULE@'))
$anchor=Get-Content "$PSScriptRoot/../legitanchoraura/RuntimeTest.cpp" -Raw
$anchor=$anchor.Substring(0,$anchor.IndexOf('// @PRODUCTION_MODULE@'))
$source=Get-Content "$client/src/minecraft/modules/combat/anchoraura/AnchorAura.cpp" -Raw
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$legacy=Get-Content "$client/src/minecraft/modules/combat/legitanchoraura/LegitAnchorAura.cpp" -Raw
$legacy=$legacy -replace '(?m)^#include[^\r\n]*\r?\n',''
$support=@'
#include "minecraft/utils/rotation/PlacementWire.h"
namespace extra {bool offhand=false,creative=false;int offhandSwings=0;}
namespace nova::java {
bool CrystalCreative(const EntityHandle&,bool& out){out=extra::creative;return true;}
bool AnchorOffhand(){return extra::offhand;}
bool MioAnchorExposure(const EntityHandle& e,const BlockPos& p,const Box& b,double& out){return AnchorExposure(e,p,b,out);}
bool SwingCrystalHand(bool offhand,bool animate){if(offhand)++extra::offhandSwings;return animate?SwingMainHand():SendSwingPacket();}
bool UseOffhandAnchor(const BlockPos& p,const BlockHitResultInfo&,int slot,const void* owner,bool& entered){
    assert(extra::offhand&&mock::charges==-1&&mock::owner==owner&&mock::slotOwner==owner&&!mock::used);
    assert(slot==(mock::spoof<0?mock::selected:mock::spoof));
    entered=mock::used=true;++mock::placed;mock::lastPlaced=p;
    if(mock::accept){mock::charges=0;mock::baseExists=true;}return mock::accept;
}
}
#define private public
'@
$test=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$out/runtime.cpp","$prefix`n$anchor`n$support`n$source`n$legacy`n#undef private`n$test")
$commands=@"
@echo off
call D:\VS2026\VS2026\VC\Auxiliary\Build\vcvars64.bat >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /I "$client/src" runtime.cpp /Fe:runtime.exe
if errorlevel 1 exit /b 1
runtime.exe
exit /b %ERRORLEVEL%
"@
[IO.File]::WriteAllText("$out/build.cmd",$commands)
Push-Location $out
try{& ./build.cmd;if($LASTEXITCODE){throw 'AnchorAura runtime regression failed'}}finally{Pop-Location}
