Contributing to WofLang Alpha-R
Thanks for your interest in improving WofLang Alpha-R (C++ v10.1.1 and Rust v0.0.1)! Contributions of all kinds are welcome: code, docs, examples, glyphmaps, plugins, and experiments.

Code Style
C++ (cpp-v10.1.1)
Target C++23.

Prefer modern C++ patterns (RAII, smart pointers, std::optional, std::variant).

Keep plugin APIs consistent with the v10.1.1 register_plugin(WoflangInterpreter&) pattern.

Avoid unnecessary global state where possible.

Match existing formatting (brace style, indentation) or run your formatter of choice consistently.

Rust (rs-v0.0.1)
Follow Rust 2021 edition conventions.

Run cargo fmt before commits.

Run cargo clippy -- -D warnings where possible.

Keep crates small and focused (woflang-core, woflang-ops, woflang-runtime, woflang-cli).

Development Workflow
Fork the repository:

bash
git clone https://github.com/whisprer/woflang-alpha-r.git
cd woflang-alpha-r
Create a feature branch:

bash
git checkout -b feature/your-feature-name
Make your changes in the relevant sub-project:

C++: cpp-v10.1.1/

Rust: rs-v0.0.1/

Use clear, conventional commit messages, for example:

feat: add trig plugin to cpp interpreter

fix: correct unit handling in Rust runtime

docs: expand plugin development guide

Push your branch and open a Pull Request against main.

Include a short description of what you changed and why.

Link to any relevant issues.

Testing
C++ Side
From cpp-v10.1.1:

bash
./clean-n-build.sh
# or:
cmake -B build
cmake --build build --config Release
ctest --test-dir build   # if tests are configured
If you add new plugins, consider adding small .wof scripts or tests under the tests directory where applicable.

Rust Side
From rs-v0.0.1:

bash
cargo build --workspace
cargo test --workspace
If you add new ops or runtime components, add unit tests or simple integration tests.

Documentation
If your change affects:

User-visible behavior.

REPL commands or glyph semantics.

Plugin APIs or Rust APIs.

Please update relevant docs:

README.md (root, C++, and/or Rust-specific READMEs).

Files under useful-docs/ (e.g., features.md, woflang_setup_guide.md, unicode_glyphmap_spec.md).

Short examples and clear explanations help others learn WofLang faster.

Issues & Communication
Use GitHub Issues for:

Bug reports.

Feature requests.

Questions or RFC-style design discussions.

When filing a bug, include:

Which implementation (C++ / Rust).

Version/commit hash.

Platform (OS, compiler, Rust version).

Minimal reproduction steps or scripts if possible.

Please keep discussion respectful and focused on the project’s goals.

Ideas Welcome
Not sure where to start? Possible contribution areas:

New glyphs or ops (math, logic, creative).

Additional plugins (fractal, audio, visualization).

Performance tuning or benchmarking.

Better error messages, tracing, or REPL UX.

Parity features between C++ and Rust.

Thank you for helping WofLang grow. 🜁