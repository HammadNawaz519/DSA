# TTDB Server — Progress Log

Tracking my progress building the Time-Travel Debugger server from scratch.

---

## Oct 1, 2026 — Session 1 · Set up the project skeleton
Created the base files, Makefile, .gitignore, empty server source.

## Oct 1, 2026 — Session 2 · Defined core data structures
Added all structs: `Token`, `Variable`, `Frame`, `Snapshot`, `FuncEntry`, `PendingPatch`.

## Oct 2, 2026 — Session 1 · Built the Timeline
Added `TimelineNode` and `Timeline` doubly-linked list. Added `TTDBHeader`.

## Oct 2, 2026 — Session 2 · Stage 1: Structural validation
Implemented `validate_structural_integrity()`.

## Oct 2, 2026 — Session 3 · Stage 2: Resolve binary generation
Implemented `generate_resolve_bin()` — reads source, writes binary records to `resolve.bin`, tracks pending call patches.

---

## Oct 3, 2026 — Session 1
**Patch call addresses and locate `main`**

Implemented `patch_resolve_bin()`. This takes every `PendingPatch` from the previous step, looks up the target function name in the function table, formats the real byte offset as an 18-char hex string, and writes it directly into `resolve.bin` at the right byte position. After patching all calls, it scans for `main` and returns its offset — that's where execution will start.
