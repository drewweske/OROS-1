#!/usr/bin/env python3
"""Repository-wide source inventory and CMake registration cross-check.

This is a mechanical structural audit, not a replacement for code review.
"""

import argparse
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, default=Path("build/oros-source-inventory.json"))
    args = parser.parse_args()
    repo = args.repo.resolve()
    cmake = repo / "CMakeLists.txt"
    if not cmake.is_file():
        parser.error("--repo must contain CMakeLists.txt")

    sources = []
    module_counts = defaultdict(Counter)
    includes = defaultdict(Counter)
    markers = []
    total_lines = 0
    for top in ("engine", "apps", "tests"):
        root = repo / top
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix not in (".cpp", ".hpp", ".h"):
                continue
            relative = path.relative_to(repo).as_posix()
            content = path.read_bytes()
            source = content.decode("utf-8-sig")
            line_count = len(source.splitlines())
            module = "/".join(relative.split("/")[:2])
            module_counts[module][path.suffix] += 1
            total_lines += line_count
            for item in re.findall(r'^\s*#include\s+"oros/([^/]+)/', source, re.M):
                includes[module][item] += 1
            for index, line in enumerate(source.splitlines(), start=1):
                if re.search(r"\b(?:TODO|FIXME|HACK)\b", line):
                    markers.append({"path": relative, "line": index, "text": line.strip()[:200]})
            sources.append({
                "path": relative,
                "sha256": hashlib.sha256(content).hexdigest(),
                "bytes": len(content),
                "lines": line_count,
            })

    cmake_content = cmake.read_text(encoding="utf-8-sig")
    cmake_cpp = re.findall(r'\b(?:engine|apps|tests)/[\w./-]+\.cpp\b', cmake_content)
    registered = Counter(cmake_cpp)
    cpp_paths = {s["path"] for s in sources if s["path"].endswith(".cpp")}
    unregistered = sorted(cpp_paths - set(registered))
    stale_registration = sorted(set(registered) - cpp_paths)
    duplicate_registration = sorted(path for path, n in registered.items() if n != 1)
    entries = {
        "source_count": len(sources),
        "cpp_count": len(cpp_paths),
        "total_source_lines": total_lines,
        "module_counts": {name: dict(counts) for name, counts in sorted(module_counts.items())},
        "oros_include_relationships": {name: dict(counts) for name, counts in sorted(includes.items())},
        "cpp_missing_from_cmake": unregistered,
        "cmake_cpp_without_source": stale_registration,
        "duplicate_cmake_cpp_registration": duplicate_registration,
        "review_markers": markers,
        "source_inventory": sources,
    }
    out = args.output if args.output.is_absolute() else repo / args.output
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(entries, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"OROS source audit: {len(sources)} source/header files, "
          f"{len(cpp_paths)} C++ translation units, {total_lines} source lines")
    print(f"CMake gaps: {len(unregistered)} missing, {len(stale_registration)} stale, "
          f"{len(duplicate_registration)} duplicates")
    print(f"Review markers: {len(markers)}; inventory: {out}")
    for kind, values in (("Unregistered", unregistered),
                         ("Missing source", stale_registration),
                         ("Duplicate", duplicate_registration)):
        for value in values:
            print(f"{kind}: {value}")
    return 1 if unregistered or stale_registration or duplicate_registration else 0


if __name__ == "__main__":
    raise SystemExit(main())
