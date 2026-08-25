#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]


def load_generator():
    path = PROJECT_ROOT / "tools" / "generate_config.py"
    spec = importlib.util.spec_from_file_location("generate_config", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ConfigurationTests(unittest.TestCase):
    def test_target_config_matches_legacy_effective_values_except_module_name(self) -> None:
        rendered = load_generator().render()
        defines = dict(
            re.findall(
                r'^#define[ \t]+(\w+)(?:[ \t]+(.*))?$', rendered, re.MULTILINE
            )
        )
        self.assertEqual('"chan_svistok"', defines["AST_MODULE"])
        self.assertEqual("1", defines["BUILD_APPLICATIONS"])
        self.assertEqual("1", defines["BUILD_MANAGER"])
        self.assertEqual("iconv_t", defines["ICONV_T"])
        self.assertEqual('"13"', defines["PACKAGE_REVISION"])
        self.assertEqual('"1.1"', defines["MODULE_VERSION"])
        self.assertNotIn("HAVE_ICONV_H", defines)
        self.assertNotIn("CONFIG_H_INCLUDED", defines)


if __name__ == "__main__":
    unittest.main()
