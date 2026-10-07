# TTDB Server — Progress Log

Tracking my progress building the Time-Travel Debugger server from scratch.

---

## Oct 1 · Session 1 — Set up project skeleton
## Oct 1 · Session 2 — Defined core data structures
## Oct 2 · Session 1 — Built the Timeline
## Oct 2 · Session 2 — Stage 1: Structural validation
## Oct 2 · Session 3 — Stage 2: Resolve binary generation
## Oct 3 · Session 1 — Patch call addresses
## Oct 3 · Session 2 — Tokenizer and variable helpers

---

## Oct 3, 2026 — Session 3
**Stage 3: The execution engine — this is the big one**

Added `read_record()` first, which seeks to a byte position in `resolve.bin` and reads one record back as a string. Then implemented the full `execute_program()`. It opens `resolve.bin`, initialises the call stack with a `main` frame, records that as step 0, then loops reading one instruction at a time.

For each instruction it dispatches based on the keyword: `set` evaluates the right-hand side and writes it to the frame, `add`/`sub`/`mul`/`div` read both operands and store the result, `call` pushes a new frame (passing evaluated argument values, saving the return address), and `func_end` pops the frame (writing back modified arguments to the caller) and jumps to the return address. After every instruction it snapshots the full call stack and appends it to the timeline. When the stack empties, execution is done.
