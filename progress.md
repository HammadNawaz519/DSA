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

Implemented `validate_structural_integrity()`. Before touching anything else, the server has to verify the source file is well-formed. This function checks that every `func` has a matching `func_end`, there are no nested function declarations, no instructions appear outside a function, and no function is left open at end of file. Any violation prints a descriptive error with the line number and returns false.
