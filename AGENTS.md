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

Do not generate or modify code unless the user explicitly requests code
changes. For exploratory or planning requests, discuss options without creating
or changing source files.

For design work, collaborate through dialogue: raise one focused question or
proposal at a time, explain tradeoffs plainly, and wait for the user's response
before treating a design choice as settled. Do not rush through multiple
unresolved decisions or turn a discussion into a finalized design without the
user's alignment.

Ask the user to clarify any unclear request, requirement, or design point
before proceeding. State assumptions explicitly; never resolve ambiguity
implicitly.

## Architectural decisions

Before adding a new language feature or dependency:

- describe the proposed behavior and limitations;
- identify its effect on elaboration and the IR;
- update the relevant documentation;
- wait for an explicit decision if the feature changes project scope.
