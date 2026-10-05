#!/usr/bin/env bash
# Builds and runs the ClassFileEditor harness end to end.
#
#   ./run.sh          everything
#   ./run.sh --quick  skip the MSVC rebuild of patch_test.exe
#
# Probe is compiled to its own directory and deliberately kept off the runtime
# class path: the test is only meaningful if the Probe the JVM defines is the
# one the rewriter produced.

set -euo pipefail
cd "$(dirname "$0")"

MC="C:/Users/13366/AppData/Roaming/.minecraft"
JAVA="$MC/runtime/java-runtime-delta/bin"
LIB="$MC/libraries"
ASM="$LIB/org/ow2/asm"
SRG="$LIB/net/minecraft/client/1.20.1-20230612.114412/client-1.20.1-20230612.114412-srg.jar"

ASM_CP="$ASM/asm/9.10.1/asm-9.10.1.jar"
ASM_CP="$ASM_CP;$ASM/asm-tree/9.10.1/asm-tree-9.10.1.jar"
ASM_CP="$ASM_CP;$ASM/asm-analysis/9.10.1/asm-analysis-9.10.1.jar"
ASM_CP="$ASM_CP;$ASM/asm-util/9.10.1/asm-util-9.10.1.jar"

if [ "${1:-}" != "--quick" ]; then
    echo "== building patch_test.exe =="
    mkdir -p build
    # The compiler's exit status is the only thing that says whether the exe
    # below is the one these sources produce. Filtering the log through a pipe
    # with `|| true` on the end -- which is what this used to do -- hides a
    # failed build behind a stale patch_test.exe that goes on passing, so the
    # status is taken from cl and a failure stops the run.
    set +e
    cmd //c "$(cygpath -w "$PWD/build.bat")" > build/build.log 2>&1
    status=$?
    set -e
    grep -Ev '^(patch_test|ClassEdit|ClassFile|audit)\.(cpp|h)$' build/build.log || true
    if [ "$status" -ne 0 ]; then
        echo "the rewriter did not build (cl exited $status); see build/build.log" >&2
        exit 1
    fi
fi

rm -rf build/probe build/driver build/out
mkdir -p build/probe build/driver build/out

echo "== compiling Probe.java =="
"$JAVA/javac.exe" -encoding UTF-8 --release 17 -g \
    -d build/probe src/Probe.java

echo "== compiling Verify.java =="
"$JAVA/javac.exe" -encoding UTF-8 --release 17 -g \
    -cp "$ASM_CP" -d build/driver src/Verify.java

echo "== running the rewriter =="
./build/patch_test.exe build/probe/Probe.class build/out classes

echo "== running the patched classes =="
# MSYS_NO_PATHCONV: the class path is a ;-separated Windows list, and letting
# the shell rewrite it turns it into something java cannot read.
MSYS_NO_PATHCONV=1 "$JAVA/java.exe" -cp "$(cygpath -w "$PWD/build/driver");$ASM_CP" \
    -Xshare:off \
    Verify "$(cygpath -w "$PWD/build/out")" "$SRG"
