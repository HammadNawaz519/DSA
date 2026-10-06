# TTDB Server — Progress Log

Tracking my progress building the Time-Travel Debugger server from scratch.

---

## Oct 1, 2026 — Session 1
**Set up the project skeleton**

Created the base files: Makefile, .gitignore, README, and an empty server source file.

---

## Oct 1, 2026 — Session 2
**Defined core data structures**

Added all the fundamental structs the project needs: `Token`, `Variable`, `Frame`, `Snapshot`, `FuncEntry`, and `PendingPatch`.

---

## Oct 2, 2026 — Session 1
**Built the Timeline — the backbone of time-travel**

Added `TimelineNode` and the `Timeline` class as a doubly-linked list. Also added `TTDBHeader`.

---

## Oct 2, 2026 — Session 2
**Stage 1: Structural validation**

Implemented `validate_structural_integrity()`. Checks that every `func` has a matching `func_end`, no nested functions, no stray instructions.

---

## Oct 2, 2026 — Session 3
**Stage 2: Generate the resolve binary**

Implemented `generate_resolve_bin()`. This reads through the source file and writes every instruction into `resolve.bin` as a binary record — each record has a byte offset, a size, and the raw line text. For `func` instructions it records the function's position in the function table. For `call` instructions it writes a hex placeholder `0x0000000000000000` where the real address will go, and registers it as a `PendingPatch`. Everything else is written as-is.
