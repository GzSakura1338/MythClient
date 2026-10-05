#!/usr/bin/env bash
# Native modules the client had loaded when it died -- an hs_err lists them all.
G="D:/MCLDownload/Game/.minecraft"
ls -la "$G"/hs_err_pid*.log
for f in "$G"/hs_err_pid*.log; do
    echo "=== $(basename "$f")"
    sed -n '/^Dynamic libraries:/,/^$/p' "$f" | grep -viE 'C:\\\\WINDOWS|\\system32|\\.minecraft\\versions|\\\.minecraft\\bin|lwjgl|jna|OpenAL|glfw|jemalloc' | head -25
done
