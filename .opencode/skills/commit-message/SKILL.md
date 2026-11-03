---
name: Commit Message
description: Format and review Lemma Git commits with required subject prefixes and explanatory bodies.
---

# Lemma Commit Message Format

Use this skill whenever preparing a Git commit in the Lemma repository.

## Required format

Every commit message must have a subject and a non-empty explanatory body,
separated by a blank line. Keep every physical subject and body line at or below
100 characters:

```text
<prefix>: <concise imperative summary>

<why the change is needed and what it changes>
<important behavior, scope, or compatibility notes>
<validation performed, when applicable>
```

Use exactly one lowercase prefix:

- `new:` — introduces a capability or component;
- `change:` — changes or refactors existing behavior or structure;
- `fix:` — corrects a defect;
- `doc:` — documentation or repository-instruction changes only.

For mixed changes, choose the prefix that describes the primary effect. Keep
the subject specific and concise; the body must explain the change rather than
repeat the subject or say only “update files.” Include test commands and results
in explanatory prose for code changes when they help explain confidence in the
change. For documentation-only changes, explain the documentation change; do
not add a generic validation footer. Never include a standalone
`Validation: git diff --check` line.

## Before committing

1. Confirm the user authorized the commit.
2. Inspect `git status --short` and review the staged diff. Stage only the
   intended files, except when the user explicitly asks to commit all changes;
   in that case, inspect and include the complete worktree deliberately.
3. Ensure relevant validation has passed; do not claim tests that were not run.
4. Commit with the required subject and body, then verify the resulting commit
   and worktree status.

Do not amend, squash, or rewrite an existing commit unless the user explicitly
asks for it.

## Examples

```text
fix: Ignore comments during RTL validation

Tree-sitter exposes comments as extra nodes, so the frontend now skips them
when validating module structure. Add a regression test for comments around
ports and assignments.

The test suite passes (7/7 tests).
```

```text
doc: Mark superseded checkpoints as historical

Add an index that points agents to the active handoff and warns against using
historical status or next-step notes as current project state.

This is a documentation-only commit.
```
