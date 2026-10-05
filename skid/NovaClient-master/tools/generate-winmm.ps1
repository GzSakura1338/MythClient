param([string]$OutputDirectory = (Join-Path $PSScriptRoot '../out/injector/native'))
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$exports = Get-Content (Join-Path $PSScriptRoot '../injector/winmm-exports.txt')
$def = [Collections.Generic.List[string]]::new()
$asm = [Collections.Generic.List[string]]::new()
$header = [Collections.Generic.List[string]]::new()
$def.Add('LIBRARY winmm'); $def.Add('EXPORTS')
$asm.Add('EXTERN NovaResolveWinmm:PROC'); $asm.Add('.code')
$header.Add("#define NOVA_WINMM_EXPORT_COUNT $($exports.Count)")
$header.Add('static const char* const exportNames[] = {')
$index = 0
foreach ($line in $exports) {
    $ordinal, $name = $line -split ' ', 2
    $symbol = "NovaWinmm$ordinal"
    if ($name -eq '-') {
        $def.Add("  $symbol @$ordinal NONAME")
        $header.Add("  reinterpret_cast<const char*>($ordinal),")
    } else {
        $def.Add("  $name=$symbol @$ordinal")
        $header.Add('  "' + $name + '",')
    }
    # 32-byte shadow space, 4 integer args and 4 vector args; stack stays aligned.
    $asm.Add(@"
$symbol PROC FRAME
    sub rsp, 88h
    .allocstack 88h
    .endprolog
    mov [rsp+20h], rcx
    mov [rsp+28h], rdx
    mov [rsp+30h], r8
    mov [rsp+38h], r9
    movdqu [rsp+40h], xmm0
    movdqu [rsp+50h], xmm1
    movdqu [rsp+60h], xmm2
    movdqu [rsp+70h], xmm3
    mov ecx, $index
    call NovaResolveWinmm
    mov rcx, [rsp+20h]
    mov rdx, [rsp+28h]
    mov r8, [rsp+30h]
    mov r9, [rsp+38h]
    movdqu xmm0, [rsp+40h]
    movdqu xmm1, [rsp+50h]
    movdqu xmm2, [rsp+60h]
    movdqu xmm3, [rsp+70h]
    add rsp, 88h
    jmp rax
$symbol ENDP
"@)
    $index++
}
$asm.Add('END'); $header.Add('};')
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'winmm.def'), $def)
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'winmm-stubs.asm'), $asm)
[IO.File]::WriteAllLines((Join-Path $OutputDirectory 'winmm-exports.h'), $header)
