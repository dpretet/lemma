# Engineering Notes

This is the index for Lemma's curated, repository-owned engineering memory. It
is intended to preserve durable decisions and implementation or debugging
details for future development and maintenance—not to keep a transcript of
every task.

The specifications and code remain authoritative. Notes supplement them with
rationale, non-obvious constraints, gotchas, and reproducible findings. If a
note conflicts with a specification or current behavior, follow the
authoritative source and update the note.

## Using and updating notes

- At the start of a C source/test task or substantive debugging investigation,
  the task preparer checks this index and reads only relevant topic notes.
- Put durable knowledge in a focused topic file in this directory and link it
  below; update the note and index together so interrupted work remains
  discoverable. Do not create a note for routine changes with no reusable
  learning.
- Record a finding as soon as it is confirmed; do not wait for the task to end.
  If interrupted, leave concise `provisional` or `open` notes with evidence and
  next steps rather than presenting an unverified conclusion as fact.
- Each entry should include its status (`accepted`, `verified`, `provisional`,
  or `open`), context/date, the decision or finding, why it matters, and links
  to relevant code, specs, tests, or reproduction evidence.
- Do not copy normative requirements from specifications, preserve raw logs or
  transcripts, or store secrets.

## Topic notes

- [Repository engineering memory](agentic-memory.md) — accepted rationale and
  boundaries for this memory process.
- [Agent plan review](agent-plan-review.md) — provisional, optional independent
  review of task-preparer handoffs before implementation.
- [SVA placement for MVP](sva-placement.md) — accepted inline-only placement
  decision and boundary for external assertion attachment.
- [RTL test oracle format](test-oracle-format.md) — accepted test-oracle layers;
  machine-readable format remains open.
- [Public API and frontend boundaries](api-boundaries.md) — accepted Tree-sitter
  encapsulation, shared source-range API, internal stage separation, job/result
  and proof-status contract, diagnostic layouts/stages, borrowed module/process
  syntax visitor with typed declaration-group/declarator events, nested grammar visitors and
  split reusable Tree-sitter helpers, CLI entry flow, direct elaboration-to-IR
  builder handoff, and stack-depth robustness priority; target-label encoding,
  serialization, one verified frontend acceptance gap, and traversal mitigation
  remain open.
- [Circuit IR storage](circuit-ir-storage.md) — accepted private C storage
  outline using one owning object and array-position IDs, plus the first
  scalar/vector alias slice, frontend-first implementation order, and direct
  builder handoff; allocator policy remains open.

The design specifications remain the source of truth. Start with the
[specification reading guide](../../README.md#specifications);
[`doc/rtl-support.md`](../rtl-support.md) is authoritative for the supported
RTL subset.
