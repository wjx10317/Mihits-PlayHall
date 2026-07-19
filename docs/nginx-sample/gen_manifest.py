#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate manifest.json for a game install directory."""
import hashlib
import json
import sys
from pathlib import Path


def md5_file(p: Path) -> str:
    h = hashlib.md5()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) < 6:
        print(
            "usage: gen_manifest.py <dir> <baseUrl> <zoneid> <exe_name> <version>",
            file=sys.stderr,
        )
        sys.exit(1)
    root = Path(sys.argv[1])
    base_url = sys.argv[2].rstrip("/") + "/"
    zoneid = int(sys.argv[3], 0)
    exe_name = sys.argv[4]
    version = sys.argv[5]
    files = []
    for p in sorted(root.rglob("*")):
        if not p.is_file() or p.name == "manifest.json":
            continue
        rel = p.relative_to(root).as_posix()
        files.append({"path": rel, "size": p.stat().st_size, "md5": md5_file(p)})
    doc = {
        "zoneid": zoneid,
        "version": version,
        "exe_name": exe_name,
        "baseUrl": base_url,
        "files": files,
    }
    out = root / "manifest.json"
    out.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("wrote", out, "files=", len(files))


if __name__ == "__main__":
    main()
