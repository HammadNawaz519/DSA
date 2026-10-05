# TTDB Server — Progress Log

Tracking my progress building the Time-Travel Debugger server from scratch.

---

## Oct 1, 2026 — Session 1
**Set up the project skeleton**

Created the base files: Makefile, .gitignore, README, and an empty server source file. Nothing runs yet — just getting organized.

---

## Oct 1, 2026 — Session 2
**Defined core data structures**

Added all the fundamental structs the project needs: `Token`, `Variable`, `Frame`, `Snapshot`, `FuncEntry`, and `PendingPatch`. Also added the `TokenType` enum and the two constants that cap how deep the call stack can go and how many variables a frame can hold. Compiles cleanly, nothing executes yet.
