#!/usr/bin/env python3
"""Delete every comment from the NovaClientBase sources.

String- and char-literal aware, so a `//` inside a string is left alone. Line
endings and indentation are preserved. Comment-only lines are removed outright;
runs of blank lines they leave behind are collapsed to one.

    python tools/strip_comments.py --dry-run     # report only
    python tools/strip_comments.py               # rewrite in place
"""

import argparse
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SCAN = [ROOT / "Client" / "src", ROOT / "Client" / "include"]

# Generated base85 font blobs and vendored third-party code. The blobs are one
# enormous string literal, and a scan that got it wrong would corrupt a font.
SKIP_DIRS = {"third_party", "build", "out"}
SKIP_GLOBS = ("font_*.h",)


def should_skip(path: pathlib.Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return True
    if any(path.match(g) for g in SKIP_GLOBS):
        return True
    # Anything that looks like generated base85 data.
    try:
        head = path.read_bytes()[:200]
    except OSError:
        return True
    return b"compressed_data_base85" in head


def strip(text: str) -> str:
    """Return `text` with every comment removed."""
    out: list[str] = []
    line: list[str] = []
    i, n = 0, len(text)

    def end_line() -> None:
        out.append("".join(line).rstrip())
        line.clear()

    while i < n:
        c = text[i]

        # A string literal, copied through verbatim so a // inside it survives.
        if c == '"':
            # Raw string: R"delim( ... )delim"
            if line and line[-1] == "R":
                j = text.find("(", i + 1)
                closer = ')' + text[i + 1:j] + '"' if j != -1 else None
                if j != -1 and closer:
                    k = text.find(closer, j)
                    if k == -1:
                        line.append(text[i:])
                        i = n
                    else:
                        line.append(text[i:k + len(closer)])
                        i = k + len(closer)
                    continue
            line.append(c)
            i += 1
            while i < n:
                ch = text[i]
                if ch == "\\" and i + 1 < n:
                    line.append(text[i:i + 2])
                    i += 2
                    continue
                if ch == "\n":          # unterminated: do not run off the edge
                    end_line()
                    i += 1
                    break
                line.append(ch)
                i += 1
                if ch == '"':
                    break
            continue

        # A character literal. '\'' and '\\' need the escape handled, or the
        # closing quote is mis-read and the rest of the line becomes a string.
        if c == "'":
            line.append(c)
            i += 1
            while i < n:
                ch = text[i]
                if ch == "\\" and i + 1 < n:
                    line.append(text[i:i + 2])
                    i += 2
                    continue
                line.append(ch)
                i += 1
                if ch == "'" or ch == "\n":
                    break
            continue

        if text.startswith("//", i):
            i += 2
            while i < n and text[i] != "\n":
                i += 1
            # A trailing backslash continues the comment onto the next line;
            # with the comment gone so is the continuation.
            while i < n and text[i] == "\n" and line and line[-1] == "\\":
                line.pop()
                end_line()
                i += 1
            if not "".join(line).strip():
                # The comment was the whole line, so the line goes with it
                # rather than leaving a blank behind.
                line.clear()
                if i < n and text[i] == "\n":
                    i += 1
            else:
                while line and line[-1] in " \t":
                    line.pop()
            continue

        if text.startswith("/*", i):
            i += 2
            while i < n and not text.startswith("*/", i):
                if text[i] == "\n":
                    end_line()
                i += 1
            i += 2
            if not "".join(line).strip():
                line.clear()
                while i < n and text[i] in " \t":
                    i += 1
                if i < n and text[i] == "\n":
                    i += 1
            continue

        if c == "\n":
            end_line()
            i += 1
            continue

        line.append(c)
        i += 1

    if line:
        end_line()

    # Collapse the blank runs that removing whole comment lines leaves behind.
    collapsed: list[str] = []
    for ln in out:
        if ln == "" and collapsed and collapsed[-1] == "":
            continue
        collapsed.append(ln)
    while collapsed and collapsed[-1] == "":
        collapsed.pop()

    return "\n".join(collapsed) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    files, before, after = 0, 0, 0
    for base in SCAN:
        for path in sorted(base.rglob("*")):
            if path.suffix not in (".cpp", ".h", ".c"):
                continue
            if should_skip(path):
                continue

            raw = path.read_bytes()
            crlf = b"\r\n" in raw
            text = raw.decode("utf-8").replace("\r\n", "\n")
            result = strip(text)
            if result == text:
                continue

            files += 1
            before += sum(1 for l in text.splitlines() if l.strip())
            after += sum(1 for l in result.splitlines() if l.strip())

            if not args.dry_run:
                path.write_bytes(result.replace("\n", "\r\n").encode("utf-8")
                                 if crlf else result.encode("utf-8"))

    verb = "would rewrite" if args.dry_run else "rewrote"
    print(f"{verb} {files} files; non-blank lines {before} -> {after}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
