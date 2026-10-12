# Open RTL Formal Verification

An open-source formal verification tool for a deliberately limited subset of
synthesizable RTL.

The project aims to let RTL designers verify small, clearly written Verilog
and SystemVerilog designs without relying on proprietary tools. The tool is
intended to be a standalone software project, not merely a front end for an
existing simulator.

## Project principles

- Define and document the supported RTL subset before implementing it.
- Reject unsupported or ambiguous constructs with clear diagnostics.
- Keep parsing, elaboration, the internal representation, and formal solving
  as distinct concerns.
- Prefer small, testable implementation steps.
- Do not silently reinterpret RTL.

## Documentation

1. [RTL support](doc/rtl-support.md) — initial Verilog/SystemVerilog subset.
2. [Agent and contributor instructions](AGENTS.md) — repository working rules.

## Status

This repository is at the architecture-definition stage. The language subset
is a working specification; items marked **TBD** require an explicit design
decision before implementation.
