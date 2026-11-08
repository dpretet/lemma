# Optional Agent Plan Review

- **Status:** provisional
- **Date:** 2026-11-07
- **Context:** The default C-change workflow separates task preparation,
  implementation, and independent post-implementation verification. A plan can
  still carry a missed requirement or assumption into implementation.
- **Decision:** The main agent may optionally ask `executor-reporter` to review a
  task-preparer handoff before implementation when scope, assumptions, or impact
  warrant an independent counterpoint. Provide the reviewer with both the
  original user request and the handoff. The review is read-only, does not run
  validation, and does not replace the main agent's decision-making or the
  executor-reporter's later verification role.
- **Rationale:** An independent comparison can expose material omissions,
  unsupported assumptions, or unresolved choices before they become code, while
  keeping the extra review discretionary for routine, clear tasks.
- **Trial boundary:** This is an optional workflow experiment, not a required
  fourth stage. Reassess whether it catches useful issues relative to its
  coordination cost after practical use.
- **Evidence:** [`AGENTS.md`](../../AGENTS.md) defines the optional review point;
  [executor-reporter](../../.opencode/agents/executor-reporter.md) defines its
  plan-review mode and keeps it distinct from post-implementation validation.
