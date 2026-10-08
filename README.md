# Time-Travel Debugger (TTDB) — Server Engine

The server-side engine for the Time-Travel Debugger (TTDB), Phase 1. This program takes a compiled TTDB bytecode binary (`source.bin`), executes it step by step, records the complete execution history as a timeline of snapshots, and serializes it to a `.tdbg` session file that a debugger client can use to step forward and backward through program execution.

---

## How It Works

The server runs the input binary through a four-stage pipeline:

```
source.bin  →  [Stage 1: Validate]  →  [Stage 2: Resolve]  →  resolve.bin
                                                                     ↓
session.tdbg  ←  [Stage 4: Serialize]  ←  [Stage 3: Execute & Snapshot]
```

| Stage | Function | Description |
|-------|----------|-------------|
| 1 | `validate_structural_integrity()` | Checks magic bytes, version, and file integrity of `source.bin` |
| 2 | `generate_resolve_bin()` | Extracts function definitions, builds a symbol table, writes `resolve.bin` |
| 3 | `patch_resolve_bin()` | Fills in call-address placeholders, locates the `main` entry point |
| 4 | `execute_program()` | Runs bytecode instructions, captures a `Snapshot` of the call stack after each step |
| 5 | `writeTdbg()` | Serializes the full `Timeline` to a binary `session.tdbg` file |

---

## Project Structure

```
.
├── ttdb_server.cpp   # Full server implementation (~790 lines, C++17)
├── source.bin        # Sample TTDB bytecode input
├── Makefile          # Build configuration
├── progress.md       # Development log — commit-by-commit breakdown
├── resolve.bin       # Generated after running (intermediate symbol table)
└── session.tdbg      # Generated after running (time-travel debug session)
```

---

## Build

Requires `g++` with C++17 support.

```bash
make
```

To clean all build artifacts and generated files:

```bash
make clean
```

---

## Run

```bash
# Use the default source.bin
./ttdb_server

# Or pass a custom source file
./ttdb_server path/to/custom.bin
```

### Successful output
```
Execution completed successfully. Total steps: 7
```

### Error output
If any stage fails, the server will print a specific error and exit:
```
Error: validation failed      # source.bin is invalid or corrupt
Error: resolve stage failed   # could not parse function definitions
Error: patching failed        # missing function referenced in a call
Error: execution failed       # runtime error during bytecode execution
```

---

## Output Files

| File | Description |
|------|-------------|
| `resolve.bin` | Intermediate binary with resolved function offsets |
| `session.tdbg` | The time-travel debug session — contains every captured snapshot |

The `session.tdbg` file starts with a `TTDBHeader` (magic bytes + step count), followed by raw binary `Snapshot` records. Each snapshot contains the full call stack state at that moment in execution.

---

## Author

**Hammad Nawaz** — hammadnawaz519@gmail.com
