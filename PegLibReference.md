# cpp-peglib reference (for gate-lang)

This document describes [cpp-peglib](https://github.com/yhirose/cpp-peglib): a **header-only** C++17 library that parses text using **PEG** (parsing expression grammars). The implementation is a single file, `peglib.h`, in namespace `peg`.

**This repo** pulls **v1.10.1** via CMake `FetchContent`, exposes the `peglib` interface target, and links it to `gate_lang` (`CMakeLists.txt`).

---

## 1. Mental model

**PEG vs. CFG.** A PEG is a set of *parsing expressions* (not ambiguous BNF). For each nonterminal, alternatives are **ordered**: `A / B` tries `A` first; only if `A` fails does it try `B`. There is no separate lexer unless you build one with rules such as `%whitespace`.

**How cpp-peglib runs.** You give a **grammar string** (or programmatic `Rules`) to `peg::parser`. The library parses that string with a built-in meta-grammar and builds an internal **grammar object** (`Grammar`: map of rule name → `Definition`). Parsing input means invoking the **start rule**’s matcher, which walks the input with **backtracking** on failure. Optional **packrat memoization** caches `(rule id, input position) → result` to avoid repeated work on the same substring.

**Two ways to get meaning from a parse:**

1. **Semantic actions** — attach C++ lambdas to rule names; each returns `std::any` (or `void`) to build values bottom-up.
2. **AST mode** — `parser.enable_ast()` installs default actions that build a tree of `peg::Ast` (or your `AstBase<Annotation>` subclass).

You can mix approaches (e.g. AST for structure, then walk the tree).

---

## 2. Minimal program shape

```cpp
#include <peglib.h>

peg::parser parser(R"(
  Start <- ...
)");
if (!parser) { /* grammar failed to compile */ }

parser["SomeRule"] = [](const peg::SemanticValues &vs) { /* ... */ };

std::any user_state;
if (parser.parse("input text", user_state)) { /* success */ }
```

- **`operator bool` on `parser`**: `true` if the grammar loaded.
- **`parser["RuleName"]`**: returns `peg::Definition &` for attaching actions, `enter`/`leave`, predicates, etc.
- **`parse`**: returns `bool`. On failure, an optional **logger** receives line/column messages (see §10).

Convenience: `parser.parse(text, value)` uses `parse_and_get_value` so `value` receives the `std::any` from the start rule’s semantic value stack (first slot), when present.

---

## 3. Grammar syntax (textual PEG)

cpp-peglib’s concrete syntax follows Ford-style PEG with extensions. The meta-grammar is effectively self-described in the upstream file `grammar/cpp-peglib.peg`.

| Construct | Meaning |
|-----------|---------|
| `Rule <- expr` | Definition (`<-` or Unicode `←`). |
| `a b` | Sequence (concatenation). |
| `a / b` | **Prioritized choice**: try `a`, then `b` if `a` fails. |
| `'lit'` `"lit"` | Literal string. |
| `[a-z]` `[^...]` | Character class; suffix `i` for case-insensitive (`'foo'i`, `[a-z]i`). |
| `.` | Any single UTF-8 code unit (library is UTF-8–oriented). |
| `expr?` `expr*` `expr+` | Optional, zero-or-more, one-or-more. |
| `expr{n}`, `expr{n,}`, `expr{,m}`, `expr{n,m}` | Bounded repetition. |
| `&expr` | And predicate: succeed if `expr` succeeds, **without consuming** input. |
| `!expr` | Not predicate: succeed if `expr` fails, **without consuming**. |
| `< expr >` | **Token boundary**: inner match is one token; affects tokens list / token AST nodes. |
| `~Rule` | Leading `~` on a **definition name** (in the grammar source) marks the rule as **ignored for AST/skip** semantics (see `Definition::ignoreSemanticValue` in the header). |
| `Rule^label` | Attach a **label** for error reporting (stored on the `Definition`). |
| `↑` | **Cut**: commit to current choice; suppress backtracking past this point in that alternative. |
| `Macro(A, B) <- ...` | **Parameterized rules** (macros); use `Macro(arg1, arg2)` at use sites. |
| `{ precedence ... }` | **Precedence climbing** block on a rule (see §6). |
| `{ message '...' }` | Custom **error message** string for that rule. |
| `{ no_ast_opt }` | Hint for AST optimizer (see §8). |

**Special definitions**

- **`%whitespace <- ...`** — Inserted automatically between tokens of **sequences** in other rules (not inside raw character classes / literals). Typical: spaces and comments.
- **`%word <- ...`** — Used when the implementation needs a “word” boundary concept in some optimizations (advanced; many grammars omit it).
- **`%recover <- ...`** — Error **recovery** expression (advanced).

**Comments:** `#` to end of line.

**Start rule:** First rule in the grammar text unless you pass an explicit start name to the `parser` constructor / `load_grammar`.

---

## 4. `peg::parser` API (selected)

| Operation | Role |
|-----------|------|
| `parser(sv)` / `load_grammar` | Load grammar; optional extra `Rules` map merges **programmatic** combinators (see §9). |
| `enable_ast<T>()` | For every rule **without** an action, install default AST builder (`T` defaults to `peg::Ast`). |
| `optimize_ast(ast, mode)` | Collapse chains of unary AST nodes using `AstOptimizer` and `{ no_ast_opt }` hints. |
| `enable_packrat_parsing()` | Turn on memoization for this grammar (see §7). |
| `enable_left_recursion(bool)` | PEG left-recursion support (default **on** in recent versions). |
| `disable_eoi_check()` | Allow parse to stop before end of input. |
| `set_logger(...)` | Receive diagnostics on failure. |
| `enable_trace` / `set_verbose_trace` | Debugging hooks; `peg::enable_tracing(parser, os)` prints a parse trace. |
| `get_grammar()` | Inspect or tweak `Definition`s programmatically. |

---

## 5. Semantic actions and `SemanticValues`

Assign to a rule: `parser["Rule"] = [](const peg::SemanticValues &vs) { ... };`

**`peg::SemanticValues`** (inherits `std::vector<std::any>`):

- **`vs[i]`** — Child semantic values in order (after pruning ignored children).
- **`vs.choice()` / `vs.choice_count()`** — Which alternative of `/` matched (0-based) and how many alternatives exist.
- **`vs.name()`** — Name of the rule whose action is running.
- **`vs.sv()`** — `string_view` of the **full span** matched by this rule.
- **`vs.token()` / `vs.tokens`** — For token rules (`< ... >`), the captured token text; `token_to_number<T>()`, `token_to_string()`.
- **`vs.line_info()`** — `{line, column}` for the match.
- **`vs.path`** — Optional path string passed into `parse`.

**Action arity** (overloads are adapted internally):

1. `(const SemanticValues &)` — typical.
2. `(const SemanticValues &, std::any &dt)` — read/write **per-parse user state** (same `dt` passed to `parse`).
3. Plus predicate arity (see §10).

Return `std::any` with a value, or `void`, or an empty `std::any`.

**`enter` / `leave` on `Definition`:** `parser["R"].enter = ...` / `.leave = ...` for side effects before/after child parsing (useful for scopes).

---

## 6. Precedence climbing

Instead of left-factoring expressions by hand, you can write:

```text
Expression <- Atom (Operator Atom)* { precedence L + - L * / }
```

The `{ precedence ... }` instruction builds a **precedence-climbing** parser for the rule. Tokens are grouped by level; `L` / `R` set left or right associativity per operator token. This pairs well with packrat on larger inputs.

---

## 7. Packrat parsing

**Idea:** Memoize parse results per `(definition_id, offset)` so repeated subcalls (common in PEG) are O(1) lookup.

**Usage:** Each `Definition` defaults to **no** packrat. Call `parser.enable_packrat_parsing()` after the grammar loads: it copies a flag from the parser (computed when the grammar was compiled—**usually `true`**, unless analysis disabled it, e.g. some back-reference cases) onto the **start rule**’s `enablePackratParsing`. Without this call, parsing stays non-memoized.

**Caveats:**

- **Memory:** Roughly proportional to `input_length × num_definitions` in worst case.
- **Semantic actions with side effects:** If an action depends on global mutable state, memoization can **skip** re-execution — side effects may run **once** where naive backtracking would run multiple times. Prefer pure actions or use `dt` carefully.
- Grammar analysis may **disable** packrat internally in some edge cases (e.g. certain back-reference setups).

---

## 8. AST mode

`parser.enable_ast();` then:

```cpp
std::shared_ptr<peg::Ast> ast;
if (parser.parse(src, ast)) {
  ast = parser.optimize_ast(ast);
}
```

**`peg::Ast`** is `AstBase<EmptyType>`: every node has `name`, `tag` (compile-time hash via `str2tag`), `nodes`, optional `token` if `is_token`, `line`, `column`, `path`, `parent`.

**Fast dispatch:** `using namespace peg::udl;` then `switch (ast->tag) { case "Add"_ : ... }` — the `_` literal produces the same `str2tag` value as `ast->tag`.

**`optimize_ast`:** Flattens useless unary chains. Rules marked `{ no_ast_opt }` control which nodes are preserved when optimizing.

**`ast_to_s(ast)`** — Debug string tree dump.

**Custom annotations:** Subclass `peg::AstBase<MyData>` and `enable_ast<MyAst>()`.

---

## 9. Programmatic rules (`Rules` / combinators)

`using Rules = std::unordered_map<std::string, std::shared_ptr<Ope>>;`

You can inject or replace rules when loading a grammar: pass a `Rules` map to the constructor / `load_grammar`. The map values are **operator objects** built with helpers such as `seq`, `cho`, `zom`, `oom`, `lit`, `cls`, `tok`, `apd`, `npd`, etc. (declared in `peglib.h`). This is useful for generated fragments or glue code without string concatenation.

**`peg::usr(fn)`** wraps a custom C function with signature compatible with the internal `Parser` callback (`size_t` length consumed or failure sentinel) for maximal control.

---

## 10. Errors, predicates, recovery

**Parse result:** Internally `Definition::Result` holds `ret`, `len`, `ErrorInfo`, and `recovered`. `parser.parse` returns `true` only for a **full success** that is not “recovered” in the error-recovery sense.

**Logger:** `parser.set_logger([](size_t line, size_t col, const std::string &msg, const std::string &rule) { ... });` — invoked for failures with location and text.

**`Definition::predicate`:** Assign a function `(const SemanticValues &, const std::any &dt, std::string &msg) -> bool` (or 4-arg form) to **reject** a match after it consumes input (semantic validation).

**`%recover`:** Supply a rule that consumes “junk” to resynchronize after errors (IDE-style tooling; understand before relying on it in compilers).

---

## 11. Left recursion

Traditional PEG cannot express direct left recursion. cpp-peglib implements an extension (toggle with `enable_left_recursion`). It integrates with the rest of the engine; if the linter reports left recursion, prefer restructuring or using precedence climbing / right-recursive idioms when you hit limits.

---

## 12. Captures and back references

**`$name< expr >`** defines a capture scope; **`$name`** later matches the same text. Packrat may be **turned off** for grammars where back references interact badly with memoization. Prefer explicit AST/token comparison in application code when possible.

---

## 13. Tooling and debugging

- **`peglint`** (optional build in upstream repo): validate a `.peg` file and optionally parse sample input, dump AST, trace, profile (`lint/README.md` in cpp-peglib).
- **`peg::enable_tracing` / `peg::enable_profiling`:** Helpers that install trace callbacks for exploratory debugging.

---

## 14. Configuration and limits

- **C++17** required (`#error` in header if below).
- **`CPPPEGLIB_HEURISTIC_ERROR_TOKEN_MAX_CHAR_COUNT`** (default 32): macro tunable before including `peglib.h` for error-message heuristics.

---

## 15. Practical tips for a hardware DSL

1. **Use `%whitespace`** for spaces and `//` / `#` comments so rules stay readable.
2. **Use `< ... >`** for identifiers, numeric literals, and operators you want as **single AST tokens** or clean `token_to_number` usage.
3. **Start with `enable_ast()`** for exploration; switch critical paths to **typed semantic actions** if you want stronger invariants or zero `std::any`.
4. **Enable packrat** if parse time grows on large files; verify actions stay **effect-free** or depend only on `dt`.
5. **Use `set_logger`** in the CLI to print **file:line:col** on syntax errors (`SemanticValues::line_info` is also available in actions).
6. **Keep alternatives ordered** — PEG choice is not commutative; put **more specific** rules before **general** ones (e.g. keywords before generic identifier).

---

## 16. Further reading

- Bryan Ford, *Parsing Expression Grammars: A Recognition-Based Syntactic Foundation* — PEG semantics.
- Upstream examples: `example/calc.cc` (actions), `example/calc3.cc` (AST), `pl0/pl0.cc` (larger AST + `str2tag` dispatch), tests under `test/`.

This file is a **project-local** summary; for new behavior or API details, prefer reading **`peglib.h`** and the version-matched **unit tests** in the same cpp-peglib tag you vendor.
