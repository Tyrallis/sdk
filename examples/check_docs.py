#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 WarcraftXL
"""Checks that every code block the docs attribute to a file is really in that file.

A tutorial whose snippets have drifted from the code teaches the wrong thing, and nothing about
compiling the example catches it: the example still builds, the text is just wrong. So each
block that is introduced by a line naming a file

    **`examples/scout/src/Journal.h`**

    ```cpp
    ...
    ```

must appear in that file as a contiguous run of lines, give or take a uniform indent. Blocks
with no file named above them are illustrations and are not checked.

    python3 examples/check_docs.py [--docs <dir>] [--root <sdk checkout>]
"""

import argparse
import re
import sys
from pathlib import Path

LABEL = re.compile(r"^\*\*`([^`]+)`\*\*\s*$")
FENCE = re.compile(r"^```([A-Za-z0-9_+-]*)\s*$")


def blocks(text):
    """Every fenced block of a document, with the file named just above it or None."""
    lines = text.split("\n")
    found = []
    label = None
    index = 0
    while index < len(lines):
        line = lines[index]
        named = LABEL.match(line)
        if named:
            label = named.group(1)
            index += 1
            continue
        if FENCE.match(line):
            start = index + 1
            index = start
            while index < len(lines) and not FENCE.match(lines[index]):
                index += 1
            found.append((label, start + 1, lines[start:index]))
            label = None
            index += 1
            continue
        if line.strip():
            label = None
        index += 1
    return found


def dedented(body):
    """The block's lines with their common leading indent removed."""
    widths = [len(line) - len(line.lstrip(" ")) for line in body if line.strip()]
    shift = min(widths) if widths else 0
    return [line[shift:] if line.strip() else "" for line in body]


def contains(source, body):
    """Whether `source` holds `body` as a contiguous run, under some uniform extra indent."""
    wanted = dedented(body)
    if not wanted:
        return True
    for first in range(len(source) - len(wanted) + 1):
        if source[first].lstrip(" ") != wanted[0].lstrip(" "):
            continue
        run = source[first:first + len(wanted)]
        if dedented(run) == wanted:
            return True
    return False


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--docs", type=Path, default=here.parent / "docs",
                        help="the directory of documents to check")
    parser.add_argument("--root", type=Path, default=here.parent,
                        help="what the paths in the labels are relative to")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    problems = []
    checked = 0
    for document in sorted(args.docs.rglob("*.md")):
        text = document.read_text(encoding="utf-8")
        where = document.relative_to(args.docs).as_posix()
        for label, line, body in blocks(text):
            if label is None:
                continue
            source = (args.root / label).resolve()
            if not source.is_file():
                problems.append("%s:%d: the block names %s, which does not exist"
                                % (where, line, label))
                continue
            checked += 1
            if contains(source.read_text(encoding="utf-8").split("\n"), body):
                if args.verbose:
                    print("ok   %s:%d  %s" % (where, line, label))
                continue
            problems.append("%s:%d: the block does not appear in %s; it starts with %r"
                            % (where, line, label, (body[0] if body else "").strip()[:60]))

    if problems:
        print("check_docs: %d problem(s)" % len(problems), file=sys.stderr)
        for problem in problems:
            print("  " + problem, file=sys.stderr)
        return 1

    print("check_docs: %d quoted block(s), every one of them really in the file it names"
          % checked)
    return 0


if __name__ == "__main__":
    sys.exit(main())
