# Instructions for Coding Agents

## Mission

Help build a standalone open-source formal verification tool for a documented,
limited subset of synthesizable Verilog/SystemVerilog RTL.

The project is currently defining its architecture and language subset.
Do not assume that an undecided feature is supported.

## Source of truth

- Treat `doc/rtl-support.md` as the working RTL-subset specification.
- If the specification is ambiguous or marked **TBD**, do not make the
  decision implicitly in code. Explain the ambiguity and propose a focused
  specification change first.
- Keep documentation and implementation behavior consistent.

## Constraints

- Do not introduce a dependency on proprietary tools.
- Keep parsing, elaboration, IR construction, and formal solving clearly
  separated.
- Do not silently accept or discard unsupported RTL.
- Avoid broad refactors unrelated to the requested task.

## Working method

For each task:

1. State the specific behavior being implemented or changed.
2. Identify affected specification sections and tests.
3. Make the smallest coherent change.
4. Add or update tests, including relevant rejection cases.
5. Report what changed, what was tested, and any unresolved issue.

Do not generate large amounts of code or documentation unless requested.
Prefer concise diagnostics and small, reviewable changes.

## Architectural decisions

Before adding a new language feature or dependency:

- describe the proposed behavior and limitations;
- identify its effect on elaboration and the IR;
- update the relevant documentation;
- wait for an explicit decision if the feature changes project scope.
