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
Implemented `generate_resolve_bin()`.

## Oct 3, 2026 — Session 1 · Patch call addresses
Implemented `patch_resolve_bin()` — fills in hex addresses for all pending call patches, returns `main` offset.

---

## Oct 3, 2026 — Session 2
**Tokenizer and variable helper functions**

Added `tokenize_line()` which splits a text instruction into a list of `Token` objects — first token is the KEYWORD, second is the IDENTIFIER, rest are PARAMs. Added `is_number()` to check if a string is a literal integer. Added `resolve_value()` which looks up a token in a frame's locals or args (or parses it as a literal). Added `set_variable()` which updates or creates a variable in a frame. These utilities are small but get called constantly by the execution engine.
