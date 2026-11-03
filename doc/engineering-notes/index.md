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
- [SVA placement for MVP](sva-placement.md) — accepted inline-only placement
  decision and boundary for external assertion attachment.
- [RTL test oracle format](test-oracle-format.md) — accepted test-oracle layers;
  machine-readable format remains open.
- [Public API and frontend boundaries](api-boundaries.md) — accepted Tree-sitter
  encapsulation, internal stage separation, job/result and diagnostic layouts;
  target-label encoding and serialization remain open.

The design specifications remain the source of truth. Start with the
[specification reading guide](../../README.md#specifications);
[`doc/rtl-support.md`](../rtl-support.md) is authoritative for the supported
RTL subset.
