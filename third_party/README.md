# Vendored dependencies

These dependency sources are compiled into Lemma; no dependency download or
Tree-sitter CLI is required to configure or build the project.

| Component | Upstream | Version | Upstream revision | License |
| --- | --- | --- | --- | --- |
| Tree-sitter C runtime | [tree-sitter/tree-sitter](https://github.com/tree-sitter/tree-sitter) | 0.25.2 | `695d1530e59804279d9c739335b73b882d9f1caf` | MIT; see `tree-sitter/LICENSE` and Unicode license in `tree-sitter/lib/src/unicode/LICENSE` |
| SystemVerilog grammar | [gmlarumbe/tree-sitter-systemverilog](https://github.com/gmlarumbe/tree-sitter-systemverilog) | 0.3.1 | `e88937e66adc3ee7be0bfe40b7e937eafe4212bb` | MIT; see `tree-sitter-systemverilog/LICENSE` |
| Unity | [ThrowTheSwitch/Unity](https://github.com/ThrowTheSwitch/Unity) | 2.6.1 | `cbcd08fa7de711053a3deec40b7e937eafe4212bb` | MIT; see `unity/LICENSE.txt` |
| Arena allocator | [tsoding/arena](https://github.com/tsoding/arena) | vendored commit | `ab625dd3ac0df8c6d82cbbcd1d8fb976ecb8b9c8` | MIT; see `tsoding-arena/LICENSE` |
| logc | [kurotych/logc](https://github.com/kurotych/logc) | upstream snapshot (2019-11-03) | `d173d7c0a12bcd2e1684071f5adcc12f81ecef6b` | MIT; see `logc/LICENSE` |
| cargs | [likle/cargs](https://github.com/likle/cargs) | upstream snapshot (2024-09-23) | `0698c3f90333446d0fc2745c1e9ce10dd4a9497a` | MIT; see `cargs/LICENSE.md` |

`logc` uses POSIX threads and requires a pthread implementation. Lemma's
current `logc` integration therefore targets POSIX/pthreads environments;
native MSVC builds require a pthread compatibility implementation.

The generated SystemVerilog parser source is stored as
`tree-sitter-systemverilog/parser-source.tar.gz` (SHA-256
`ca223c0cced691bb836bb885e84eff9b79fb4e3a3c2cea2aa5a474b241441d86`). CMake
extracts it to the build directory when needed; incremental builds reuse the
extracted file. The archive contains only the generated `src/parser.c`. The
Tree-sitter CLI is only needed if maintainers choose to regenerate that source.
Grammar recognition is a syntax-front-end capability, not a declaration that
Lemma supports every SystemVerilog construct.
