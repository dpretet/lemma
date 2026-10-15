# Lemma

Lemma is an open-source project to build a standalone formal verification tool
for a documented, deliberately limited subset of synthesizable Verilog and
SystemVerilog RTL. Its goal is to make useful RTL proofs possible without
depending on proprietary tools.

> **Current status:** Lemma is at the parser/build-bootstrap stage, not yet a
> formal verification tool. The current `lemma` executable checks SystemVerilog
> syntax only. It does not validate the supported RTL subset, elaborate a
> design, parse or check assertions semantically, or prove properties. The
> specifications below describe the intended behavior, not features already
> implemented.

## Build, test, and try it

Requirements: CMake 3.16 or newer, a C11 compiler, POSIX threads, and Bash to
run `flow.sh`.

```sh
./flow.sh build    # configure and build a Debug version
./flow.sh test     # build and run the tests
./flow.sh release  # build/test Release and package the executable
./flow.sh clean    # remove generated build/ and dist/ files
```

Parse the included valid fixture, or substitute your own `.sv` file:

```sh
./build/lemma tests/fixtures/valid.sv
```

The current command accepts exactly one source path. An optional `--` may
precede it to mark the end of options. It exits successfully after a syntax
parse, or reports a syntax/file error and exits unsuccessfully. It does not run
a proof. The test suite includes parser unit tests and valid and malformed
syntax fixtures.

The release command creates `dist/lemma-<platform>-<architecture>.tar.gz` for
the host OS and architecture. The archive contains the executable, a brief
`README.txt`, and bundled license notices. Build separately on each platform
you want to distribute for.

## Project direction

The planned MVP is a small end-to-end verifier for a single flat top module,
checking one assertion or cover at a time with optional assumptions. It is
intended to use bounded model checking (BMC) and k-induction: a bounded search
without a counterexample alone must not be reported as a proof. The exact
solver/backend and several language and event-semantics details are still open.

The project is being developed in small, specified and tested increments:

- Keep parsing, elaboration, internal representation, and formal solving
  separate.
- Reject unsupported or ambiguous constructs rather than silently ignoring or
  reinterpreting them.
- Treat the documented subset—not all syntax recognized by the parser—as the
  authority for future implementation.
- Do not depend on proprietary tools.

## Specifications

These documents describe the intended scope and semantics; they do not imply
that those features are implemented yet. Read them in this order for a
book-like introduction to Lemma's design. The existing documents remain the
authoritative sources; this chapter order is only a reading guide. Historical
checkpoints and backup documents are intentionally excluded.

### Part I — Scope and input language

1. [MVP specification](doc/mvp.md) — first milestone and acceptance boundary.
2. [RTL support](doc/rtl-support.md) — supported language subset, open
   questions, and conformance-test roadmap.

### Part II — Circuit and temporal semantics

3. [Internal representation](doc/internal-representation.md) — elaborated
   circuit structure, combinational logic, and FFD state elements.
4. [Register semantics](doc/register-semantics.md) — initialization, reset,
   state updates, and retention.
5. [Execution and assertion semantics](doc/execution-and-assertion-semantics.md)
   — proof-state transitions and clock-domain model.
6. [SVA support](doc/sva-support.md) — assertion, assumption, cover, and
   temporal-operator subset.

### Part III — Proof flow and evidence

7. [Tool flow](doc/tool-flow.md) — planned processing stages and results.
8. [Proof model](doc/proof-model.md) — intended BMC and k-induction strategy.
9. [Validation criteria](doc/validation-criteria.md) — evidence expected as
   features are implemented.


## Dependencies and license

Lemma is distributed under the [MIT License](LICENSE).

Tree-sitter, its SystemVerilog grammar, Unity, the `tsoding/arena` allocator,
`kurotych/logc`, and `likle/cargs` are vendored under [`third_party/`](third_party/).
