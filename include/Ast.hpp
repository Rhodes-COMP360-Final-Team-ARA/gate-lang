/**
 * File: Ast.hpp
 * Purpose: Syntax tree for the GateLang v1 prototype — components only, untyped names.
 *
 * No bus widths, imports, slices, or concatenation: only what you need to see recursive
 * gate expressions and multi-output instantiation. Widths and DAG lowering stay in
 * later compiler passes.
 */
#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace gate::ast {

struct Span {
  std::string path;
  std::size_t line = 1;
  std::size_t column = 1;
};

struct ReturnStmt {
  Span span;
  std::vector<std::string> names;
};

enum class BinOp { And, Or, Xor };

struct Expr;

/**
 * shared_ptr: cpp-peglib stores semantic values in std::any (copyable). See prototype.md.
 */
using ExprPtr = std::shared_ptr<Expr>;

struct IdentExpr {
  Span span;
  std::string name;
};

struct CallExpr {
  Span span;
  std::string component;
  std::vector<std::string> args;
};

struct NotExpr {
  Span span;
  ExprPtr inner;
};

struct BinaryExpr {
  Span span;
  BinOp op = BinOp::And;
  ExprPtr left;
  ExprPtr right;
};

struct ParenExpr {
  Span span;
  ExprPtr inner;
};

struct Expr {
  std::variant<IdentExpr, CallExpr, NotExpr, BinaryExpr, ParenExpr> node;

  explicit Expr(decltype(node) n) : node(std::move(n)) {}
};

struct Assignment {
  Span span;
  std::vector<std::string> lhs;
  ExprPtr rhs;
};

struct ComponentDecl {
  Span span;
  std::string name;
  std::vector<std::string> params;
  std::vector<Assignment> body;
  ReturnStmt ret;
};

struct Program {
  std::vector<ComponentDecl> components;
};

void print_program(const Program &program, std::ostream &os);

} // namespace gate::ast
