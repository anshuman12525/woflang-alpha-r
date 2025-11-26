WofLang Alpha-R Changelog
This changelog tracks major changes for the WofLang Alpha-R repository, which hosts both:

cpp-v10.1.1 – C++23 implementation of the WofLang interpreter and plugins.
rs-v0.0.1 – Early Rust port of the core, runtime, ops, and CLI.

Format: Keep newer versions at the top. Use semantic-ish versioning per track where practical.

cpp-v10.1.1
v10.1.1 – Modern Plugin API & Mathlib (2025-11-26)
MILESTONE: Stabilized C++ interpreter and plugin API for WofLang Alpha-R.

Highlights:

Standardized plugin interface:
register_plugin(WoflangInterpreter& interp) entry point.
WoflangPlugin base class with register_ops(...).

Updated core math plugins:
mathlib_exponentials.cpp: exp, ln, log2, log10 with numeric type safety.
mathlib_calculus.cpp: derivative/integral hooks (stubs in base version).

Symbolic math and logic:
symbolic_logic_ops.cpp: logical ops (and, or, not, implies, iff).
ops_symbolic_simplify_rules.cpp: basic pattern simplification (e.g. X + X → 2 * X).
ops_symbolic_solve_linear.cpp: simple ax = b solver.
symbolic_solve.cpp: quadratic solver demo.

Trigonometric & hyperbolic functions:
trig_ops.cpp: sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh, deg2rad, rad2deg.

Stack and “forbidden” ops:
stack_slayer_op.cpp: dramatic stack clearing.
ops_forbidden_stack_slayer.cpp: alternate stack-slaying behavior.
void_division_op.cpp / stack_void_divison.cpp: void-themed operations.

Quality of life:
Unified error messages for stack underflow and type mismatch.
Consistent use of WofValue::make_* constructors (int/double).
Clean separation of core (src/core), REPL (src/repl), and plugins (plugins/).

cpp-v9.0.0 first fully working useable language

cpp-v8.5.0 inreasesin uccessful fucntionality
cpp-v8.0.0 first successes at full working program

cpp-v7.0.0 attempts to get a working whole

cpp-v6.0.0 introduction of analog and SIMD

cpp-v5.0.0 total rewrite to allow new features

cpp-v4.0.0 introduction of new core setup

cpp-v3.0.0 plguins become hotswappable

cpp-v2.0.0 further developments in plugins

cpp-v1.0.0 other features like overloading

cpp-v0.1.0 proper attempts at memory management

asm-v0.0.5 lgyphs and mdularising

asm-v0.0.4 ohilosophy grew introducing math and symbols

asm-v0.0.3 developments in ideas such as easter eggs etc.

asm-v0.0.2 first attempt to create a real proper lang in code

py-v0.0.1 intial idea

rs-v0.0.1
v0.0.1 – Initial Rust Workspace (2025-11-26)
MILESTONE: First Rust-based WofLang workspace layout.

Workspace structure:

crates/woflang-core:
Stack machine (stack.rs), Value representation (value.rs).
Opcode definitions (opcode.rs), instructions (instruction.rs).
Block, scope, and unit handling modules.

crates/woflang-ops:
Core ops: arithmetic, logic, math, crypto, stack, IO, constants, experimental quantum stubs.

crates/woflang-runtime:
Interpreter (interpreter.rs), tokenizer (tokenizer.rs), registry and plugin traits.

crates/woflang-cli:
Basic CLI driver with minimal REPL loop and script execution.

Goals of this release:
Establish a stable Rust API surface for future feature parity with the C++ implementation.
Provide a clean separation of concerns: core types, operations, runtime, and CLI.
Enable early adopters to experiment with Rust-based WofLang components.

Repository-Level
Initial Alpha-R Unification (2025-11-26)
Created woflang-alpha-r repo to host both C++ v10.1.1 and Rust v0.0.1 implementations.

Added root README.md describing:
Dual-implementation architecture.
Feature matrix.
Build steps and usage for both ecosystems.

Imported and organized documentation into useful-docs/:
Language philosophy.
Unicode glyphmap spec.
Setup guides, feature registry, Easter eggs, and roadmap.

Added project meta files:

CODE_OF_CONDUCT.md
CONTRIBUTING.md
SECURITY.md
CHANGELOG.md (this file)
Roadmap (High-Level)
C++ (cpp-v10.1.1)
Improved trace mode with glyph-level introspection.
Additional math, graph, and symbolic plugins.
More robust unit system and dimensional analysis.

Rust (rs-v0.0.1)
Achieve feature parity with core C++ interpreter behavior.
Expose plugin-style extension mechanisms similar to the C++ plugin API.
WASM and server-side integration experiments.