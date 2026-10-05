$ErrorActionPreference='Stop'
$client=[IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$out="$client/build-vs2026/test-autocrystal"
New-Item -ItemType Directory -Force $out | Out-Null
# Reuse the JNI/event/slot fixture; run the actual independent production module.
$fixture=Get-Content "$PSScriptRoot/../legitcrystalaura/RuntimeTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$source=Get-Content "$client/src/minecraft/modules/combat/autocrystal/AutoCrystal.cpp" -Raw
$source=$source -replace '(?m)^#include[^\r\n]*\r?\n',''
$support=@'
#include "minecraft/modules/combat/autocrystal/MioPolicy.h"
#include "minecraft/modules/combat/autocrystal/ResistanceTrace.h"
#include "minecraft/utils/rotation/PlacementWire.h"
#define NOVA_TEST_PLACEMENT_WIRE
namespace extra {bool offhand=false,creative=false;}
namespace nova::java {
bool CrystalCreative(const EntityHandle&,bool& out){out=extra::creative;return true;}
bool CrystalOffhand(){return extra::offhand;}
bool PlaceOffhandEndCrystal(const BlockHitResultInfo& hit,const void* owner,bool& entered){
    assert(extra::offhand&&CrystalPlaceable(hit.blockPos)&&mock::owner==owner&&!mock::used&&!mock::motion);
    assert(mock::attack.Acquire(owner));mock::used=mock::locked=entered=true;++mock::uses;return true;
}
bool SwingCrystalHand(bool,bool animate){return animate?SwingMainHand():SendSwingPacket();}
bool MioCrystalExposureAt(const EntityHandle& e,const rotation::Vec3& point,const Box& bounds,const BlockPos* base,double& exposure) {
    return CrystalExposureAt(e,point,bounds,base,exposure);
}
}
#define private public
'@
$fixture=$fixture.Replace('// @PRODUCTION_MODULE@',"$support`n$source`n#undef private")
$fixture=$fixture.Replace('MakeLegitCrystalAuraModule','MakeAutoCrystalModule')
$tests=Get-Content "$PSScriptRoot/RuntimeTest.cpp" -Raw
[IO.File]::WriteAllText("$out/runtime.cpp","$fixture`n$tests")
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
try{& ./build.cmd;if($LASTEXITCODE){throw 'AutoCrystal runtime regression failed'}}finally{Pop-Location}
