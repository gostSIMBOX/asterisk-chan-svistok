# Legacy Characterization Harness

The harness compiles source files directly from the read-only legacy tree while
placing every driver, object, executable, and result under `build/characterization`.
It runs the approved source guard both before and after every oracle executable.

Tests are added and frozen one case at a time. Golden fixtures describe legacy
behavior; they are not copied from legacy test files.

`golden-manifest.json` records every fixture hash, source file, and covered
symbol. `../../manifests/effective-legacy-config.json` records the exact host
toolchain, normalization boundary, and explicit T3/T4 gaps. Queue and call
state tests compile the complete legacy translation units with recorder stubs;
they do not duplicate the tested function bodies.
