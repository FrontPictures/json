# Documentation

This manual documents the compiled JSON fork and its source layout under
`src/nlohmann/`. Start with the [overview](mkdocs/docs/index.md), use the
[API reference](mkdocs/docs/api.md), or follow the [CMake guide](mkdocs/docs/cmake.md).
The [migration guide](mkdocs/docs/migration.md) lists the differences from upstream.

## Build the site

From the repository root, install the documentation dependencies into a virtual
environment and build with strict link/navigation validation.

```sh
python3 -m venv docs/mkdocs/venv
docs/mkdocs/venv/bin/python -m pip install -r docs/mkdocs/requirements.txt
docs/mkdocs/venv/bin/python -m mkdocs build --strict -f docs/mkdocs/mkdocs.yml
```

On Windows, use `docs/mkdocs/venv/Scripts/python.exe` for the virtual environment's
Python executable. The generated site is written to `docs/mkdocs/site/`.
To preview it, use `python -m mkdocs serve -f docs/mkdocs/mkdocs.yml` with that
environment activated. The existing Makefile supports `make -C docs/mkdocs
install_venv`, `build`, and `serve` on systems with Make.

## Compile the examples

The examples are standalone CMake consumers of the library's exported target.
They validate basic use, file input/output, binary encodings, and incremental
serializer output.

```sh
cmake -S docs/examples -B build/docs-examples -DCMAKE_BUILD_TYPE=Release
cmake --build build/docs-examples --config Release --parallel 2
ctest --test-dir build/docs-examples -C Release --output-on-failure
```

The Markdown code examples and these programs describe the current fork.
Historical upstream release notes remain in the root `ChangeLog.md`.
