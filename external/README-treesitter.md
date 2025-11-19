# Tree-sitter Integration (optional)

This project includes an optional Tree-sitter integration for structural parsing and richer highlighting.
It is disabled by default and builds cleanly without any external dependencies.

## Quick start

1) Vendor Tree-sitter core and grammars under `external/` (recommended as Git submodules):

```powershell
cd C:\Users\jaakk\editor-demo
mkdir external
cd external
# Core
git submodule add https://github.com/tree-sitter/tree-sitter.git
# C and C++ grammars
git submodule add https://github.com/tree-sitter/tree-sitter-c.git
git submodule add https://github.com/tree-sitter/tree-sitter-cpp.git

# (Optional) Add more:
# git submodule add https://github.com/tree-sitter/tree-sitter-python.git
# git submodule add https://github.com/tree-sitter/tree-sitter-javascript.git
# git submodule add https://github.com/tree-sitter/tree-sitter-typescript.git (and subtree grammar)
# git submodule add https://github.com/tree-sitter/tree-sitter-json.git
# git submodule add https://github.com/tree-sitter/tree-sitter-yaml.git
# git submodule add https://github.com/tree-sitter/tree-sitter-markdown.git
```

2) Configure with Tree-sitter enabled:

```powershell
cd C:\Users\jaakk\editor-demo\build
cmake -DENABLE_TREESITTER=ON ..
ninja editor_gui
```

- CMake will auto-detect and build the core and C/C++ grammars if present under `external/`.
- Additional grammars can be wired similarly by extending `CMakeLists.txt` with their `parser.c` (and optional `scanner.c/cc`).

## Runtime use
- The editor already passes full document text to the highlighter and will prefer Tree-sitter tokens if the backend is available.
- Languages are picked by filename extension; if a supported grammar is not vendored, the legacy incremental tokenizer is used.

## Extending the bridge
- The current bridge (`include/treesitter_bridge.h`, `src/treesitter_bridge.cpp`) is a stub.
- After linking the Tree-sitter runtime and grammars, implement:
  - Parser creation per language (one per document)
  - Incremental reparse on edits
  - Convert nodes (or query captures) to `Token` spans per line

This keeps the project functional today while enabling a smooth path to full Tree-sitter power.
