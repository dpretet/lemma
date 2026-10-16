# Instructions for Coding Agents

## Project and implementation status

Lemma is an open-source formal verification tool for a documented, limited
subset of synthesizable Verilog/SystemVerilog RTL. The repository is primarily
a C11 project. Its current implementation is at the parser/build-bootstrap
stage: the `lemma` executable checks SystemVerilog syntax, but does not yet
validate the supported RTL subset, elaborate designs, check assertions
semantically, or prove properties. Do not describe planned behavior as already
implemented.

## C implementation

- Follow the existing C style and the C11 configuration in `CMakeLists.txt`;
  keep changes warning-clean under the configured compiler options.
- Match the repository's simple house style: four-space indentation,
  `snake_case` function names, and the surrounding brace convention. Do not
  reformat unrelated code just to apply a style preference.
- Make ownership and lifetime expectations clear at API boundaries, and release
  resources on all applicable paths.
- Check input and library-operation failures, and handle cleanup consistently on
  error paths. Make size arithmetic and conversions between integer types
  explicit and safe.
- Keep functions focused and interfaces small. Prefer straightforward C over
  unnecessary abstraction, hidden state, or unrelated cleanup.
- Match surrounding formatting and naming. Document public API contracts,
  especially ownership, valid inputs, and failure behavior.
- Keep public interfaces in `include/lemma/` and implementation in the existing
  `src/` structure. Avoid broad refactors unrelated to the task.
- Keep parsing, elaboration, IR construction, and formal solving as distinct
  stages; do not pull future-stage responsibilities into the parser.
- Do not edit generated build outputs or vendored `third_party/` code unless the
  task explicitly requires it.

### C safety review reference

Use the [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)
as a review reference, not as a requirement for full compliance. Focus on
applicable guidance for array and string bounds, integer overflow and
conversions, memory ownership and lifetime, error handling, and API contracts.
For review findings, explain the concrete risk and cite a CERT rule or
recommendation when confident; distinguish must-fix issues from lower-priority
recommendations. Do not report a violation mechanically without checking that
it applies to the code.

### C change go/no-go check

Before considering a C change complete, check that:

- it stays within the user's requested scope and preserves unrelated behavior;
- ownership, lifetime, error handling, and integer-boundary behavior are clear;
- relevant success and failure cases are covered by tests;
- it builds without warnings under the configured options and `./flow.sh test`
  passes.

Do not call a change ready if it introduces compiler warnings, leaves resource
cleanup unclear, weakens or removes tests to obtain a pass, or silently changes
behavior outside the request. Report blockers rather than hiding them.

## RTL behavior and specifications

- Treat `doc/rtl-support.md` as the authority for the supported RTL subset.
  Consult the relevant design documents under `doc/` for semantics and
  architecture.
- If a requirement is ambiguous or marked **TBD**, do not decide it implicitly
  in code. Explain the ambiguity and propose a focused specification change;
  wait for alignment when the choice affects project scope.
- Do not silently accept, reinterpret, or discard unsupported RTL. Add relevant
  rejection tests when implementing RTL-subset behavior.
- Do not introduce dependencies on proprietary tools.

## Working method and verification

For user-authorized C source or test changes, use the project agents by default
in this order: `task-preparer` → `builder` → `executor-reporter`. The main
agent coordinates the handoffs and gives each agent the preceding result.

- If the task preparer identifies an ambiguity, unresolved design choice, or
  scope change, stop and get the user's alignment before implementation.
- For a clear task already authorized by the user, do not ask for redundant
  approval just to begin building.
- Have the builder implement and perform its applicable verification. Then have
  the executor-reporter independently run the exact handed-off validation
  commands and explain the logs; do not run these stages concurrently.
- Skip the trio for explanations, exploration, review-only requests, and
  documentation-only edits. Follow a user's explicit request to use or skip a
  stage when it differs from this default.

For implementation work:

1. State the specific behavior or C change being made.
2. Identify affected code, documentation/specification sections, and tests.
3. Make the smallest coherent change and keep docs consistent with behavior.
4. Add or update tests, including rejection cases where relevant.
5. Run `./flow.sh test` for code or test changes. This configures and builds the
   Debug tree before running CTest.
6. Report what changed, the commands run and their outcomes, and any unresolved
   issues.

Use `./flow.sh build` to build without running tests. Use `./flow.sh release`
for release/package work. `./flow.sh clean` removes `build/` and `dist/`; do
not use it as routine verification.

## Change authorization

- Write or modify code only when the user explicitly requests implementation or
  other code changes. A request to explain, inspect, review, or plan is not
  permission to edit code.
- Keep edits within the requested scope; do not bundle in unrequested cleanup,
  refactoring, or behavior changes.
- Do not delete existing code or tests, remove behavior, or delete files unless
  the user explicitly authorizes that removal. If removal seems necessary but
  was not requested, explain why and ask first.
- Keep code, diagnostics, and documentation concise and reviewable. For design
  work, raise one focused question or proposal at a time, explain tradeoffs, and
  wait for the user's alignment before treating a choice as settled. Ask for
  clarification when a request or requirement is unclear; state assumptions
  explicitly.
