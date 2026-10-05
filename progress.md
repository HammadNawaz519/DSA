# TTDB Server — Progress Log

Tracking my progress building the Time-Travel Debugger server from scratch.

---

## Oct 1, 2026 — Session 1
**Set up the project skeleton**

Created the base files: Makefile, .gitignore, README, and an empty server source file.

---

## Oct 1, 2026 — Session 2
**Defined core data structures**

Added all the fundamental structs the project needs: `Token`, `Variable`, `Frame`, `Snapshot`, `FuncEntry`, and `PendingPatch`. Also added the `TokenType` enum and the two constants.

---

## Oct 2, 2026 — Session 1
**Built the Timeline — the backbone of time-travel**

Added `TimelineNode` and the `Timeline` class. It's a doubly-linked list where each node stores a full `Snapshot`. Because it's doubly linked, you'll be able to walk forward and backward through execution history later. Also added `TTDBHeader` which defines what goes at the top of the `.tdbg` output file.
