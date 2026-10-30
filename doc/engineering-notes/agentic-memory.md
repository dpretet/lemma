# Repository Engineering Memory

- **Status:** accepted
- **Date:** 2026-10-30
- **Context:** Lemma needs to preserve non-obvious architecture and implementation
  knowledge across development and debugging sessions for future maintainers.
- **Decision:** Keep curated, version-controlled Markdown notes in the
  repository. At the start of C source/test work or substantive debugging, the
  task preparer retrieves only relevant notes. Record confirmed durable
  decisions and findings incrementally, with evidence and a clear status, rather
  than waiting for session end or keeping a transcript of every task.
- **Rationale:** Repository notes are reviewable and survive session/model
  changes. Incremental checkpoints reduce loss if a session is interrupted;
  status labels prevent provisional findings from being mistaken for settled
  decisions.
- **Boundary:** Notes supplement current code and specifications, which remain
  authoritative. Do not copy normative requirements into memory or treat an
  unverified diagnosis as fact. See [`AGENTS.md`](../../AGENTS.md) and the
  [memory index](index.md) for the operating procedure.
