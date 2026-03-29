/**
 * File: Ast.cpp
 * Purpose: Debug printing for the prototype AST.
 */
#include "Ast.hpp"

#include <type_traits>

namespace gate::ast {

namespace {

const char *binop_str(BinOp op) {
  switch (op) {
  case BinOp::And:
    return "AND";
  case BinOp::Or:
    return "OR";
  case BinOp::Xor:
    return "XOR";
  }
  return "?";
}

void indent(std::ostream &os, int depth) {
  for (int i = 0; i < depth; ++i) {
    os << "  ";
  }
}

void print_span(std::ostream &os, const Span &s) {
  if (!s.path.empty()) {
    os << s.path << ":";
  }
  os << s.line << ":" << s.column;
}

void print_expr(std::ostream &os, const Expr &e, int depth);

void print_expr(std::ostream &os, const ExprPtr &p, int depth) {
  if (p) {
    print_expr(os, *p, depth);
  } else {
    indent(os, depth);
    os << "(null expr)\n";
  }
}

void print_expr(std::ostream &os, const Expr &e, int depth) {
  std::visit(
      [&](auto &&alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, IdentExpr>) {
          indent(os, depth);
          os << "Ident \"" << alt.name << "\" @ ";
          print_span(os, alt.span);
          os << "\n";
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          indent(os, depth);
          os << "Call \"" << alt.component << "\" @ ";
          print_span(os, alt.span);
          os << "\n";
          for (const auto &a : alt.args) {
            indent(os, depth + 1);
            os << "arg \"" << a << "\"\n";
          }
        } else if constexpr (std::is_same_v<T, NotExpr>) {
          indent(os, depth);
          os << "NOT @ ";
          print_span(os, alt.span);
          os << "\n";
          print_expr(os, alt.inner, depth + 1);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          indent(os, depth);
          os << "Binary " << binop_str(alt.op) << " @ ";
          print_span(os, alt.span);
          os << "\n";
          print_expr(os, alt.left, depth + 1);
          print_expr(os, alt.right, depth + 1);
        } else if constexpr (std::is_same_v<T, ParenExpr>) {
          indent(os, depth);
          os << "Paren @ ";
          print_span(os, alt.span);
          os << "\n";
          print_expr(os, alt.inner, depth + 1);
        }
      },
      e.node);
}

} // namespace

void print_program(const Program &program, std::ostream &os) {
  os << "Program (components only, untyped names)\n";
  for (const auto &comp : program.components) {
    indent(os, 1);
    os << "comp " << comp.name << " @ ";
    print_span(os, comp.span);
    os << "\n";
    indent(os, 2);
    os << "params\n";
    for (const auto &p : comp.params) {
      indent(os, 3);
      os << p << "\n";
    }
    indent(os, 2);
    os << "body\n";
    for (const auto &st : comp.body) {
      indent(os, 3);
      os << "assign @ ";
      print_span(os, st.span);
      os << "\n";
      indent(os, 4);
      os << "lhs\n";
      for (const auto &n : st.lhs) {
        indent(os, 5);
        os << n << "\n";
      }
      indent(os, 4);
      os << "rhs\n";
      if (st.rhs) {
        print_expr(os, *st.rhs, 5);
      }
    }
    indent(os, 2);
    os << "return @ ";
    print_span(os, comp.ret.span);
    os << "\n";
    for (const auto &n : comp.ret.names) {
      indent(os, 3);
      os << n << "\n";
    }
  }
}

} // namespace gate::ast
