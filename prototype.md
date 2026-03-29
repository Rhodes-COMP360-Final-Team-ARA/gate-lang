# GateLang parser prototype (v1 — minimal)

This document describes the **streamlined prototype**: **only component definitions**, **untyped identifiers** (no `:width`), and expressions built from **primitives + calls + parentheses**. It matches `Parser.cpp`, `Ast.hpp`, and `examples/prototype_minimal.gate`.

---

## 1. What this grammar is for

- **Team clarity:** One top-level form (`comp`), one name shape (`IDENT`), one assignment shape (`names = expr`).
- **Pipeline stub:** Parse → AST → (later) semantic pass / DAG / simulation. Widths and imports are intentionally omitted.
- **Still “real” parsing:** Multi-output LHS, nested `NOT` / `AND` / `OR` / `XOR`, recursive **instantiation** `Name(a, b)`.

---

## 2. Language (informal)

- A **program** is zero or more **components**.
- A **component** is: `comp` *Name* `(` *parameters* `)` `{` *body* `}`.
- **Parameters** are comma-separated identifiers, or empty.
- **Body:** zero or more assignments, then one **`return`** listing output names.
- **Assignment:** one or more identifiers on the left, `=`, an **expression**, `;`.
- **Expression:** usual precedence — `OR` loosest, then `XOR`, then `AND`, then unary `NOT`, then **primary**.
- **Primary:** a **call** `Foo(a, b)` or `Foo()`, an identifier, or `( expr )`.

**Keywords** (not valid as `IDENT`): `comp`, `return`, `NOT`, `AND`, `OR`, `XOR`.

**Comments:** `//` to end of line (via `%whitespace`).

---

## 3. Grammar (EBNF-style)

```txt
program       ::= component*

component     ::= 'comp' IDENT '(' parameters ')' '{' body '}'

parameters    ::= /* empty */
                | IDENT (',' IDENT)*

body          ::= statement* return_stmt

statement     ::= ident_list '=' expression ';'

return_stmt   ::= 'return' ident_list ';'

ident_list    ::= IDENT (',' IDENT)*

expression    ::= or_expr

or_expr       ::= xor_expr ('OR' xor_expr)*
xor_expr      ::= and_expr ('XOR' and_expr)*
and_expr      ::= unary ('AND' unary)*

unary         ::= 'NOT' unary | primary

primary       ::= call | IDENT | '(' expression ')'

call          ::= IDENT '(' arguments ')'

arguments     ::= /* empty */
                | IDENT (',' IDENT)*
```

---

## 4. AST (`include/Ast.hpp`)

| Type | Role |
|------|------|
| `Program` | `std::vector<ComponentDecl>` |
| `ComponentDecl` | name, param names, body, `ReturnStmt` |
| `Assignment` | `lhs` name list, `ExprPtr rhs` |
| `Expr` | `variant` of `IdentExpr`, `CallExpr`, `NotExpr`, `BinaryExpr`, `ParenExpr` |

**`ExprPtr` = `std::shared_ptr<Expr>`** because cpp-peglib puts results in `std::any`, which requires **copyable** types.

---

## 5. Parser implementation notes (`src/Parser.cpp`)

- **Start symbol:** first rule, `program`.
- **`%whitespace`:** `[ \t\r\n]* ('//' …)*` — sequence form avoids first-set issues with `/` between spaces and comments (see older prototype notes).
- **`KEY` / `IDENT`:** `!KEY` so keywords are never identifiers; order `XOR` before `OR`.
- **`param_list`:** only the **inside** of `(` `)`; optional `IDENT` list.
- **`primary`:** **call** before **IDENT** so `Foo(` is not read as ident + garbage.
- **`call`:** two alternatives — with args vs `()` empty.

---

## 6. CLI

```bash
cmake -S . -B build && cmake --build build
./build/gate-lang examples/prototype_minimal.gate
```

---

## 7. Restoring the full language later

Reintroduce incrementally: `import`, `name:width`, slices `[lo:hi]`, `{a,b}` concat, file-level items, then semantic width checking. The **expression layering** and **call** shape stay the same idea.
