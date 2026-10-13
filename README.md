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

## What works today

- A C11/CMake build produces the `lemma` command-line executable.
- The executable parses one SystemVerilog source file using vendored
  Tree-sitter sources, reports syntax errors with source locations, and reports
  successful parses.
- Unity unit tests and parser smoke tests run through CTest.
- Configuration and builds use vendored dependencies and do not download
  dependencies.

The parser grammar recognizes syntax beyond Lemma's intended RTL subset. A
successful parse therefore does **not** mean that the input is supported or
suitable for formal verification.

## Build, test, and try it

Requirements: CMake 3.16 or newer, a C11 compiler, and Bash to run `flow.sh`.

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

The current command accepts exactly one source path. It exits successfully
after a syntax parse, or reports a syntax/file error and exits unsuccessfully.
It does not run a proof. The test suite includes parser unit tests and valid and
malformed syntax fixtures.

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

## Specifications and project docs

These documents describe the intended scope and semantics; they do not imply
that those features are implemented yet.

- [MVP specification](doc/mvp.md) — first milestone and acceptance boundary.
- [RTL support](doc/rtl-support.md) — language subset, open questions, and
  incremental conformance-test roadmap.
- [Internal representation](doc/internal-representation.md) — elaborated
  circuit structure, combinational logic paths, and FFD state elements.
- [SVA support](doc/sva-support.md) — intended assertion, assumption, cover,
  and temporal-operator subset.
- [Register semantics](doc/register-semantics.md) — initialization, reset,
  state updates, and retention.
- [Execution and assertion semantics](doc/execution-and-assertion-semantics.md)
  — proof-state and clock-domain model.
- [Proof model](doc/proof-model.md) — intended BMC and k-induction strategy.
- [Tool flow](doc/tool-flow.md) — planned processing stages and results.
- [Validation criteria](doc/validation-criteria.md) — evidence expected as
  features are implemented.
- [Vendored dependencies](third_party/README.md) — versions and licenses.
- [Contributor instructions](AGENTS.md) — repository rules for development.

## Dependencies and license

Tree-sitter, its SystemVerilog grammar, Unity, and the `tsoding/arena` allocator
are vendored under [`third_party/`](third_party/). The allocator is currently
not used by the parser. See the dependency notes for upstream versions and
license details. Lemma is distributed under the [MIT License](LICENSE).
