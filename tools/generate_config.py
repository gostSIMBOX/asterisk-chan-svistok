#!/usr/bin/env python3
"""Generate the target-owned effective configuration header."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = PROJECT_ROOT / "config" / "svistok_config.h.in"


def render(module_name: str = "chan_svistok") -> str:
    template = TEMPLATE.read_text(encoding="utf-8")
    rendered = template.replace("@AST_MODULE@", module_name)
    if "@" in rendered:
        raise RuntimeError("unresolved configuration placeholder")
    return rendered


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--module", default="chan_svistok")
    arguments = parser.parse_args()
    try:
        arguments.output.parent.mkdir(parents=True, exist_ok=True)
        arguments.output.write_text(render(arguments.module), encoding="utf-8")
    except (OSError, RuntimeError) as error:
        print(f"configuration generation failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
