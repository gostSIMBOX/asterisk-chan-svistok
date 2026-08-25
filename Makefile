PYTHON ?= python3
BUILD_DIR ?= build/module

.PHONY: module compatibility-module clean

module:
	@test -n "$(ASTERISK_INCLUDE)" || (echo "ASTERISK_INCLUDE is required" >&2; exit 2)
	$(PYTHON) tools/build_module.py --build-dir "$(BUILD_DIR)" --asterisk-include "$(ASTERISK_INCLUDE)"

compatibility-module:
	$(PYTHON) tools/build_module.py --build-dir "$(BUILD_DIR)" --asterisk-include tests/characterization/stubs

clean:
	@echo "Generated output is under $(BUILD_DIR); remove it explicitly when desired."
