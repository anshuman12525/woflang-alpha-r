//! Core interpreter for the Woflang stack machine.
//!
//! The [`Interpreter`] executes Woflang source code by tokenizing input
//! and dispatching operations through the registry. It maintains the
//! execution state (stack, scopes) and provides the context for operation handlers.

use crate::{Registry, Token, TokenKind, Tokenizer};
use std::collections::VecDeque;
use std::fs;
use std::io::{self, BufRead, Write};
use std::path::Path;
use woflang_core::{
    BlockId, BlockRegistry, BlockStack, BlockType, InterpreterContext, 
    Result, ScopeStack, Span, WofError, WofStack, WofValue,
};

/// The Woflang interpreter.
///
/// Manages the execution state and operation dispatch for a Woflang
/// program. The interpreter owns both the stack and the operation
/// registry, providing a complete execution environment.
///
/// # Examples
///
/// ```
/// use woflang_runtime::Interpreter;
///
/// let mut interp = Interpreter::new();
/// interp.exec_line("42 17 +").unwrap();
///
/// let result = interp.stack().peek().unwrap().as_integer().unwrap();
/// assert_eq!(result, 59);
/// ```
pub struct Interpreter {
    /// The data stack.
    stack: WofStack,
    /// The return stack (for function calls).
    return_stack: Vec<usize>,
    /// Operation registry.
    registry: Registry<Self>,
    /// Variable scopes.
    scopes: ScopeStack,
    /// Block registry (for control flow).
    blocks: BlockRegistry,
    /// Block nesting stack.
    block_stack: BlockStack,
    /// Token buffer for lookahead/control flow.
    token_buffer: VecDeque<OwnedToken>,
    /// Current instruction pointer (for compiled mode).
    ip: usize,
    /// Skip mode depth (for skipping else branches etc).
    skip_depth: usize,
    /// Debug mode: print stack after each line.
    pub debug: bool,
}

/// An owned token for buffering during control flow.
#[derive(Debug, Clone)]
pub struct OwnedToken {
    /// The kind of token.
    pub kind: TokenKind,
    /// The token text (owned).
    pub text: String,
    /// Source location.
    pub span: Span,
}

impl<'a> From<Token<'a>> for OwnedToken {
    fn from(t: Token<'a>) -> Self {
        Self {
            kind: t.kind,
            text: t.text.to_string(),
            span: t.span,
        }
    }
}

impl Default for Interpreter {
    fn default() -> Self {
        Self::new()
    }
}

impl Interpreter {
    /// Create a new interpreter with an empty registry.
    #[must_use]
    pub fn new() -> Self {
        Self {
            stack: WofStack::with_capacity(64),
            return_stack: Vec::with_capacity(16),
            registry: Registry::new(),
            scopes: ScopeStack::new(),
            blocks: BlockRegistry::new(),
            block_stack: BlockStack::new(),
            token_buffer: VecDeque::new(),
            ip: 0,
            skip_depth: 0,
            debug: false,
        }
    }

    /// Create an interpreter with a pre-configured registry.
    #[must_use]
    pub fn with_registry(registry: Registry<Self>) -> Self {
        Self {
            stack: WofStack::with_capacity(64),
            return_stack: Vec::with_capacity(16),
            registry,
            scopes: ScopeStack::new(),
            blocks: BlockRegistry::new(),
            block_stack: BlockStack::new(),
            token_buffer: VecDeque::new(),
            ip: 0,
            skip_depth: 0,
            debug: false,
        }
    }

    /// Get a reference to the registry.
    #[must_use]
    pub fn registry(&self) -> &Registry<Self> {
        &self.registry
    }

    /// Get a mutable reference to the registry.
    #[must_use]
    pub fn registry_mut(&mut self) -> &mut Registry<Self> {
        &mut self.registry
    }

    /// Register an operation handler.
    pub fn register<F>(&mut self, name: impl Into<String>, handler: F)
    where
        F: Fn(&mut Self) -> Result<()> + Send + Sync + 'static,
    {
        self.registry.register(name, handler);
    }

    // ═══════════════════════════════════════════════════════════════
    // VARIABLE ACCESS
    // ═══════════════════════════════════════════════════════════════

    /// Get the scope stack.
    #[must_use]
    pub fn scopes(&self) -> &ScopeStack {
        &self.scopes
    }

    /// Get mutable access to the scope stack.
    pub fn scopes_mut(&mut self) -> &mut ScopeStack {
        &mut self.scopes
    }

    /// Define a variable in the current scope.
    pub fn define_var(&mut self, name: impl Into<String>, value: WofValue) {
        self.scopes.define(name, value);
    }

    /// Get a variable's value.
    pub fn get_var(&self, name: &str) -> Result<WofValue> {
        self.scopes.get_var(name)
    }

    /// Set a variable's value (must already exist).
    pub fn set_var(&mut self, name: &str, value: WofValue) -> Result<()> {
        self.scopes.set_var(name, value)
    }

    /// Check if a variable is defined.
    #[must_use]
    pub fn has_var(&self, name: &str) -> bool {
        self.scopes.is_defined(name)
    }

    // ═══════════════════════════════════════════════════════════════
    // BLOCK & SCOPE MANAGEMENT
    // ═══════════════════════════════════════════════════════════════

    /// Push a new scope for a block.
    pub fn push_scope(&mut self, block_type: BlockType) -> BlockId {
        let block_id = self.blocks.register(
            block_type,
            self.ip,
            Some(self.block_stack.current()),
            Span::synthetic(),
        );
        self.block_stack.push(block_id);
        if block_type.creates_scope() {
            self.scopes.push(block_id);
        }
        block_id
    }

    /// Pop the current scope.
    pub fn pop_scope(&mut self) {
        if let Some(block_id) = self.block_stack.pop() {
            if let Some(block) = self.blocks.get(block_id) {
                if block.block_type.creates_scope() {
                    self.scopes.pop();
                }
            }
            self.blocks.close(block_id, self.ip);
        }
    }

    /// Get the current block depth.
    #[must_use]
    pub fn block_depth(&self) -> usize {
        self.block_stack.depth()
    }

    // ═══════════════════════════════════════════════════════════════
    // RETURN STACK (for function calls)
    // ═══════════════════════════════════════════════════════════════

    /// Push a return address.
    pub fn push_return(&mut self, addr: usize) {
        self.return_stack.push(addr);
    }

    /// Pop a return address.
    pub fn pop_return(&mut self) -> Option<usize> {
        self.return_stack.pop()
    }

    // ═══════════════════════════════════════════════════════════════
    // EXECUTION
    // ═══════════════════════════════════════════════════════════════

    /// Execute a single line of Woflang code.
    ///
    /// The line is tokenized and each token is dispatched through the
    /// interpreter. Errors are returned immediately; partial execution
    /// may have modified the stack.
    pub fn exec_line(&mut self, line: &str) -> Result<()> {
        let trimmed = line.trim();
        if trimmed.is_empty() {
            return Ok(());
        }

        // Buffer all tokens for lookahead
        let tokenizer = Tokenizer::new(trimmed);
        self.token_buffer.clear();
        for token in tokenizer {
            self.token_buffer.push_back(token.into());
        }

        // Process tokens
        while let Some(token) = self.token_buffer.pop_front() {
            self.dispatch_owned_token(&token)?;
        }

        if self.debug {
            eprintln!("[debug] stack: {}", self.stack);
            eprintln!("[debug] scope depth: {}", self.scopes.depth());
        }

        Ok(())
    }

    /// Execute a script from a file.
    pub fn exec_file(&mut self, path: impl AsRef<Path>) -> Result<()> {
        let content = fs::read_to_string(path.as_ref()).map_err(WofError::from)?;
        for line in content.lines() {
            self.exec_line(line)?;
        }
        Ok(())
    }

    /// Run an interactive REPL (Read-Eval-Print Loop).
    ///
    /// Reads lines from stdin and executes them. Errors are printed
    /// but do not terminate the REPL.
    pub fn repl(&mut self) -> io::Result<()> {
        let stdin = io::stdin();
        let mut stdout = io::stdout();

        writeln!(stdout, "Woflang REPL v{}. Type 'exit' to quit.", woflang_core::VERSION)?;

        let reader = stdin.lock();
        for line in reader.lines() {
            let line = line?;
            let trimmed = line.trim();

            if trimmed == "exit" || trimmed == "quit" {
                writeln!(stdout, "Goodbye from woflang! 🐺")?;
                break;
            }

            if trimmed == ".s" || trimmed == "." {
                writeln!(stdout, "{}", self.stack)?;
                continue;
            }

            if trimmed == ":scope" || trimmed == ":vars" {
                let names = self.scopes.all_visible_names();
                writeln!(stdout, "Variables: {}", names.join(", "))?;
                continue;
            }

            match self.exec_line(&line) {
                Ok(()) => {
                    if !self.stack.is_empty() {
                        if let Ok(top) = self.stack.peek() {
                            writeln!(stdout, "→ {top}")?;
                        }
                    }
                }
                Err(e) => {
                    writeln!(stdout, "Error: {e}")?;
                }
            }
        }

        Ok(())
    }

    /// Dispatch an owned token.
    fn dispatch_owned_token(&mut self, token: &OwnedToken) -> Result<()> {
        // If we're in skip mode, only process block delimiters
        if self.skip_depth > 0 {
            return self.handle_skip_mode(token);
        }

        match token.kind {
            TokenKind::Integer => {
                let value: i64 = token.text.parse()?;
                self.stack.push(WofValue::integer(value));
            }
            TokenKind::Float => {
                let value: f64 = token.text.parse()?;
                self.stack.push(WofValue::double(value));
            }
            TokenKind::String => {
                let value = crate::tokenizer::parse_string_literal(&token.text);
                self.stack.push(WofValue::string(value));
            }
            TokenKind::Symbol => {
                self.dispatch_symbol(&token.text)?;
            }
            TokenKind::Label => {
                // Label definition (:name) - register for jump targets
                let name = token.text.trim_start_matches(':');
                // Store current position as label target
                // For now, just acknowledge it
                if self.debug {
                    eprintln!("[debug] label defined: {name}");
                }
            }
            TokenKind::LabelRef => {
                // Label reference (@name) - for jumps
                let name = token.text.trim_start_matches('@');
                self.stack.push(WofValue::symbol(format!("@{name}")));
            }
            TokenKind::Eof => {}
        }
        Ok(())
    }

    /// Handle tokens while in skip mode (skipping else branches etc).
    fn handle_skip_mode(&mut self, token: &OwnedToken) -> Result<()> {
        match token.text.as_str() {
            "⺆" | "若" | "loop" | "⟳" => {
                // Nested block - increase skip depth
                self.skip_depth += 1;
            }
            "⺘" => {
                // Block close - decrease skip depth
                self.skip_depth = self.skip_depth.saturating_sub(1);
            }
            "或" if self.skip_depth == 1 => {
                // We hit the else branch at our skip level - stop skipping
                self.skip_depth = 0;
            }
            _ => {
                // Skip this token
            }
        }
        Ok(())
    }

    /// Dispatch a symbol (operation or identifier).
    fn dispatch_symbol(&mut self, name: &str) -> Result<()> {
        // Check for variable read syntax: 読 varname or just varname if it exists
        if name == "読" || name == "load" || name == "get" {
            // Next token should be variable name
            if let Some(next) = self.token_buffer.pop_front() {
                if next.kind == TokenKind::Symbol {
                    let value = self.get_var(&next.text)?;
                    self.stack.push(value);
                    return Ok(());
                }
                // Put it back if not a symbol
                self.token_buffer.push_front(next);
            }
            return Err(WofError::Runtime("読 requires a variable name".into()));
        }

        // Check for variable define syntax: 字 varname value
        if name == "字" || name == "define" || name == "let" {
            if let Some(next) = self.token_buffer.pop_front() {
                if next.kind == TokenKind::Symbol {
                    let var_name = next.text.clone();
                    // Value comes from stack
                    let value = self.stack.pop()?;
                    self.define_var(var_name, value);
                    return Ok(());
                }
                self.token_buffer.push_front(next);
            }
            return Err(WofError::Runtime("字 requires a variable name".into()));
        }

        // Check for variable set syntax: 支 varname
        if name == "支" || name == "set" || name == "store" {
            if let Some(next) = self.token_buffer.pop_front() {
                if next.kind == TokenKind::Symbol {
                    let value = self.stack.pop()?;
                    self.set_var(&next.text, value)?;
                    return Ok(());
                }
                self.token_buffer.push_front(next);
            }
            return Err(WofError::Runtime("支 requires a variable name".into()));
        }

        // Check for conditionals: 若 (if)
        if name == "若" || name == "if" {
            let condition = self.stack.pop()?;
            let is_true = condition.is_truthy();
            
            if is_true {
                // Execute the then branch, will skip else when we hit 或
                self.push_scope(BlockType::If);
            } else {
                // Skip until we hit 或 (else) or ⺘ (end)
                self.skip_depth = 1;
            }
            return Ok(());
        }

        // Check for else: 或
        if name == "或" || name == "else" {
            // If we're here, we executed the then branch - skip the else
            self.skip_depth = 1;
            return Ok(());
        }

        // Check for block delimiters
        if name == "⺆" {
            self.push_scope(BlockType::Generic);
            return Ok(());
        }

        if name == "⺘" {
            self.pop_scope();
            return Ok(());
        }

        // Clone the handler Arc to avoid borrow conflict
        if let Some(op) = self.registry.get_cloned(name) {
            return op(self);
        }

        // Check if it's a defined variable - auto-load it
        if self.has_var(name) {
            let value = self.get_var(name)?;
            self.stack.push(value);
            return Ok(());
        }

        // Not found: push as symbol
        self.stack.push(WofValue::symbol(name));
        Ok(())
    }
}

// ═══════════════════════════════════════════════════════════════════════
// InterpreterContext IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════

impl InterpreterContext for Interpreter {
    #[inline]
    fn push(&mut self, value: WofValue) {
        self.stack.push(value);
    }

    #[inline]
    fn pop(&mut self) -> Result<WofValue> {
        self.stack.pop()
    }

    #[inline]
    fn peek(&self) -> Result<&WofValue> {
        self.stack.peek()
    }

    #[inline]
    fn has(&self, n: usize) -> bool {
        self.stack.has(n)
    }

    #[inline]
    fn stack(&self) -> &WofStack {
        &self.stack
    }

    #[inline]
    fn stack_mut(&mut self) -> &mut WofStack {
        &mut self.stack
    }

    #[inline]
    fn clear(&mut self) {
        self.stack.clear();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_interp() -> Interpreter {
        let mut interp = Interpreter::new();

        // Register basic ops for testing
        interp.register("+", |ctx| {
            let b = ctx.stack_mut().pop_numeric()?;
            let a = ctx.stack_mut().pop_numeric()?;
            ctx.push(WofValue::double(a + b));
            Ok(())
        });

        interp.register("-", |ctx| {
            let b = ctx.stack_mut().pop_numeric()?;
            let a = ctx.stack_mut().pop_numeric()?;
            ctx.push(WofValue::double(a - b));
            Ok(())
        });

        interp.register("dup", |ctx| ctx.stack_mut().dup());
        interp.register("drop", |ctx| ctx.stack_mut().drop());
        interp.register("swap", |ctx| ctx.stack_mut().swap());

        interp
    }

    #[test]
    fn exec_arithmetic() {
        let mut interp = make_interp();
        interp.exec_line("5 3 +").unwrap();

        let result = interp.stack.pop_numeric().unwrap();
        assert!((result - 8.0).abs() < f64::EPSILON);
    }

    #[test]
    fn exec_stack_ops() {
        let mut interp = make_interp();
        interp.exec_line("42 dup").unwrap();

        assert_eq!(interp.stack.len(), 2);
        assert_eq!(interp.stack.pop_integer().unwrap(), 42);
        assert_eq!(interp.stack.pop_integer().unwrap(), 42);
    }

    #[test]
    fn exec_swap() {
        let mut interp = make_interp();
        interp.exec_line("1 2 swap").unwrap();

        assert_eq!(interp.stack.pop_integer().unwrap(), 1);
        assert_eq!(interp.stack.pop_integer().unwrap(), 2);
    }

    #[test]
    fn unknown_symbol_pushed() {
        let mut interp = make_interp();
        interp.exec_line("undefined_op").unwrap();

        let val = interp.stack.pop().unwrap();
        assert_eq!(val.as_str().unwrap(), "undefined_op");
    }

    #[test]
    fn parse_string_literal() {
        let mut interp = make_interp();
        interp.exec_line(r#""hello world""#).unwrap();

        let val = interp.stack.pop().unwrap();
        assert_eq!(val.as_str().unwrap(), "hello world");
    }

    #[test]
    fn empty_line_noop() {
        let mut interp = make_interp();
        interp.exec_line("").unwrap();
        interp.exec_line("   ").unwrap();

        assert!(interp.stack.is_empty());
    }
}
