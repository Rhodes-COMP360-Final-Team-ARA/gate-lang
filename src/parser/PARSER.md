# GateLang parser

This document describes how the GateLang parser is wired up, how the PEG grammar is structured, and how to extend it safely.

## Where things live

| Piece | Location |
| --- | --- |
| Grammar (PEG string) | `src/parser/Parser.cpp` — `kGrammar` |
| Semantic actions | `src/parser/Parser.cpp` — `attach_actions()` |
| AST types | `include/parser/Ast.hpp` |
| Public API | `include/parser/Parser.hpp` — `parse_program()` |

The grammar is embedded as a C++ raw string so it ships with the binary. There is no separate `.peg` file.

## Library

Parsing uses [cpp-peglib](https://github.com/yhirose/cpp-peglib): a packrat PEG parser with semantic actions bound to rule names.

## Program structure (non-expression rules)

At a high level, a source file is:

- zero or more `import` statements
- one or more `comp` definitions

Each component has a parameter list (`var_init` entries: `name : width`) and a `body` of statements: component calls, initializations, mutations, or `return`.

Expression parsing is only one slice of the grammar; the rest is mostly straightforward ordered choices and lists.

## Expression precedence chain

Expressions are parsed by a **chain of named rules**. Each rule owns one precedence level and **delegates downward** to the next (tighter-binding) level.

Precedence increases as you go down the chain:

```text
expr          logic binary ops (AND / OR / XOR)     ← lowest precedence
  └─ shift_expr   shifts (<< / >>)
       └─ unary       prefix NOT
            └─ postfix     postfix slice [lo:hi]
                 └─ atom        literals, var refs, parens, merges  ← highest
```

Rough mental model: `a AND b << 2` groups as `a AND (b << 2)` because shifts bind tighter than `AND`.

## Left associativity without left recursion

PEG parsers (and peglib’s default) do **not** support direct left recursion. A rule like `expr <- expr op term` would recurse forever.

Instead, left-associative infix levels use an **iterative** pattern:

```text
level <- tighter_level (op tighter_level)*
```

The semantic action receives a **flat** list of children, e.g. for `a AND b OR c`:

```text
[Expr(a), AND, Expr(b), OR, Expr(c)]
```

and **left-folds** them into a binary tree: `(a AND b) OR c`. That gives left associativity without left recursion.

`expr` (logic) and `shift_expr` (shifts) both use this pattern.

## Right-recursive prefix (`NOT`)

Prefix unary uses right recursion:

```text
unary <- 'NOT' unary / atom
```

So `NOT NOT x` parses as `NOT (NOT x)`, which is the usual expectation for stacked prefix operators.

## Ordered choice in `atom` (PEG gotcha)

`atom` is an ordered choice of leaf alternatives. The **first match wins**.

`IDENT` must stay **after** more specific forms (parentheses, merge `{…}`, binary literal `0b…`). Otherwise any identifier-shaped token would be swallowed by `IDENT` before a dedicated rule (including future keywords or sigils) could run.

For any new bracketed or sigil-led leaf form, add a **named sub-rule** (like `merge_expr`) and list it in `atom` **before** `IDENT`.

## Postfix operators on `postfix`

`postfix` is not an ordered choice — it is `atom` followed by an **optional postfix suffix**. Currently the only suffix is the slice `[lo:hi]`:

```text
postfix <- atom ('[' INT ':' INT ']')?
```

Because postfix requires the base to be parsed first, PEG cannot express it with a choice inside the same rule (that would need left recursion). The two-level split (`postfix` / `atom`) is the standard PEG workaround.

When the suffix is absent `vs.size() == 1` and `postfix` passes the `atom` Expr through unchanged. When the suffix is present `vs.size() == 3` and `postfix` wraps the base in a `SplitExpr`.

To add a second postfix operator (e.g. a future type-cast), extend `postfix` with another optional suffix and handle the new `vs.size()` case in its action.

## Current expression-related rules (summary)

| Rule | Role |
| --- | --- |
| `expr` | Logic: `shift_expr (bin_operator shift_expr)*` |
| `bin_operator` | `'AND' / 'OR' / 'XOR'` |
| `shift_expr` | `unary (shift_op INT)*` — shift amount is a decimal `INT` |
| `shift_op` | `'<<' / '>>'` |
| `unary` | `'NOT' unary / postfix` |
| `postfix` | `atom ('[' INT ':' INT ']')?` — optional postfix bit-slice |
| `atom` | `'(' expr ')' / merge_expr / LITERAL / IDENT` — ordered leaf choices |
| `merge_expr` | `'{' expr (',' expr)+ '}'` — at least two operands |
| `LITERAL` | Binary literal: `0b` followed by bits |
| `IDENT`, `INT` | As documented in the grammar string |

The authoritative spelling of every rule is always `kGrammar` in `Parser.cpp`.

## How to add new expression syntax

### New infix operator at an **existing** precedence level

1. Extend that level’s operator rule (e.g. add `'NAND'` to `bin_operator`).
2. Update the action for that level to map the new token to the right AST node / enum arm.

### New infix operator at a **new** precedence level

1. Invent a rule name, e.g. `cmp_expr`.
2. **Insert** it between the two levels it should sit between.
3. Rewire neighbours so the chain stays connected, for example:

   ```text
   expr     <- cmp_expr (bin_operator cmp_expr)*    # was: shift_expr …
   cmp_expr <- shift_expr (cmp_op shift_expr)*      # new
   shift_expr <- …                                   # unchanged below
   ```

4. Define `cmp_op` (or reuse an existing operator rule).
5. Add a `cmp_expr` action that left-folds like `expr` / `shift_expr`.

### New prefix unary operator

Add an alternative **before** the `atom` fallback in `unary`:

```text
unary <- 'NOT' unary / 'NEG' unary / atom
```

Then extend the `unary` action with a new `vs.choice()` branch (choice indices shift when you reorder alternatives — update comments accordingly).

### New terminal or composite leaf

Add an alternative in `atom` **before** `IDENT`, ideally as its own rule for a clean action.

### New postfix operator

Extend the optional suffix group in `postfix` and add a new `vs.size()` branch in the `postfix` action. If the operator has its own distinct syntax, give it a named sub-rule (e.g. `type_cast`) so the action stays readable.

## Merge expressions

`merge_expr` requires **at least two** comma-separated inner expressions. A lone `{ a }` is rejected by the grammar (`+` on the repeated segment), which matches an n-ary merge with no useful arity-1 case.

## Actions and `SemanticValues`

Actions are registered on rule names, e.g. `pg["expr"] = …`.

Conventions that tend to work well:

- **`vs.choice()`** — which alternative of an ordered choice matched (when the rule’s top-level production is `/`).
- **`vs[i]`** — the *i*-th child’s semantic value (after child rules have run).
- **`vs.transform<T>()`** — when every child is the same type (e.g. statement lists).
- **Left folds** — loop `i = 1; i < vs.size(); i += 2` for `expr op expr op …` shapes.

When you add rules, keep child order documented in the action’s comment so the `vs[…]` indices stay obvious.

## See also

- `syntax.md` at repo root (language surface, if present and up to date)
- `include/parser/Ast.hpp` for the exact `Expr` variant arms actions must build
