/**
 * File: Parser.cpp
 * Purpose: Minimal GateLang prototype — only `comp` definitions, plain identifiers (no
 * `:width`), booleans + calls + parens. See prototype.md.
 */
#include "Parser.hpp"

#include <peglib.h>

#include <any>
#include <stdexcept>
#include <utility>

namespace gate {

namespace {

using namespace peg;
using ast::Assignment;
using ast::BinOp;
using ast::BinaryExpr;
using ast::CallExpr;
using ast::ComponentDecl;
using ast::Expr;
using ast::ExprPtr;
using ast::IdentExpr;
using ast::NotExpr;
using ast::ParenExpr;
using ast::Program;
using ast::ReturnStmt;
using ast::Span;

Span span_of(const SemanticValues &vs) {
  auto loc = vs.line_info();
  return Span{vs.path ? std::string(vs.path) : std::string(), loc.first,
              loc.second};
}

ExprPtr make_binary(Span s, BinOp op, ExprPtr left, ExprPtr right) {
  return std::make_shared<Expr>(
      BinaryExpr{s, op, std::move(left), std::move(right)});
}

ExprPtr fold_binops(const SemanticValues &vs, BinOp op) {
  if (vs.empty()) {
    throw std::runtime_error("internal: empty binary fold");
  }
  ExprPtr acc = std::any_cast<ExprPtr>(vs[0]);
  const Span s = span_of(vs);
  for (std::size_t i = 1; i < vs.size(); ++i) {
    ExprPtr rhs = std::any_cast<ExprPtr>(vs[i]);
    acc = make_binary(s, op, std::move(acc), std::move(rhs));
  }
  return acc;
}

struct BodyBundle {
  std::vector<Assignment> stmts;
  ReturnStmt ret;
};

void attach_actions(peg::parser &parser) {

  parser["program"] = [](const SemanticValues &vs) -> Program {
    Program program;
    program.components.reserve(vs.size());
    for (const auto &cell : vs) {
      program.components.push_back(std::any_cast<ComponentDecl>(cell));
    }
    return program;
  };

  parser["component"] = [](const SemanticValues &vs) -> ComponentDecl {
    ComponentDecl c;
    c.span = span_of(vs);
    c.name = std::any_cast<std::string>(vs[0]);
    c.params = std::any_cast<std::vector<std::string>>(vs[1]);
    auto body = std::any_cast<BodyBundle>(vs[2]);
    c.body = std::move(body.stmts);
    c.ret = std::move(body.ret);
    return c;
  };

  parser["param_list"] = [](const SemanticValues &vs) -> std::vector<std::string> {
    std::vector<std::string> params;
    params.reserve(vs.size());
    for (const auto &cell : vs) {
      params.push_back(std::any_cast<std::string>(cell));
    }
    return params;
  };

  parser["body"] = [](const SemanticValues &vs) -> BodyBundle {
    if (vs.empty()) {
      throw std::runtime_error("component body must end with return");
    }
    BodyBundle bb;
    bb.ret = std::any_cast<ReturnStmt>(vs.back());
    bb.stmts.reserve(vs.size() - 1);
    for (std::size_t i = 0; i + 1 < vs.size(); ++i) {
      bb.stmts.push_back(std::any_cast<Assignment>(vs[i]));
    }
    return bb;
  };

  parser["stmt"] = [](const SemanticValues &vs) -> Assignment {
    Assignment a;
    a.span = span_of(vs);
    a.lhs = std::any_cast<std::vector<std::string>>(vs[0]);
    a.rhs = std::any_cast<ExprPtr>(vs[1]);
    return a;
  };

  parser["ident_list"] = [](const SemanticValues &vs) -> std::vector<std::string> {
    std::vector<std::string> ids;
    ids.reserve(vs.size());
    for (const auto &cell : vs) {
      ids.push_back(std::any_cast<std::string>(cell));
    }
    return ids;
  };

  parser["return_stmt"] = [](const SemanticValues &vs) -> ReturnStmt {
    ReturnStmt r;
    r.span = span_of(vs);
    r.names = std::any_cast<std::vector<std::string>>(vs[0]);
    return r;
  };

  parser["or_expr"] = [](const SemanticValues &vs) -> ExprPtr {
    return fold_binops(vs, BinOp::Or);
  };

  parser["xor_expr"] = [](const SemanticValues &vs) -> ExprPtr {
    return fold_binops(vs, BinOp::Xor);
  };

  parser["and_expr"] = [](const SemanticValues &vs) -> ExprPtr {
    return fold_binops(vs, BinOp::And);
  };

  parser["unary"] = [](const SemanticValues &vs) -> ExprPtr {
    if (vs.choice() == 0) {
      return std::make_shared<Expr>(
          NotExpr{span_of(vs), std::any_cast<ExprPtr>(vs[0])});
    }
    return std::any_cast<ExprPtr>(vs[0]);
  };

  parser["primary"] = [](const SemanticValues &vs) -> ExprPtr {
    switch (vs.choice()) {
    case 0:
      return std::any_cast<ExprPtr>(vs[0]); // call
    case 1: {
      Span s = span_of(vs);
      return std::make_shared<Expr>(
          IdentExpr{s, std::any_cast<std::string>(vs[0])});
    }
    default:
      return std::make_shared<Expr>(
          ParenExpr{span_of(vs), std::any_cast<ExprPtr>(vs[0])});
    }
  };

  parser["call"] = [](const SemanticValues &vs) -> ExprPtr {
    Span s = span_of(vs);
    std::string name = std::any_cast<std::string>(vs[0]);
    std::vector<std::string> args;
    if (vs.choice() == 0) {
      for (std::size_t i = 1; i < vs.size(); ++i) {
        args.push_back(std::any_cast<std::string>(vs[i]));
      }
    }
    return std::make_shared<Expr>(
        CallExpr{s, std::move(name), std::move(args)});
  };

  parser["IDENT"] = [](const SemanticValues &vs) -> std::string {
    return vs.token_to_string();
  };
}

static constexpr const char *kGrammar = R"(
# v1 prototype: program = list of components; identifiers have no widths. See prototype.md.
program      <- component*

component    <- 'comp' IDENT '(' param_list ')' '{' body '}'

param_list   <- (IDENT (',' IDENT)*)?

body         <- stmt* return_stmt

stmt         <- ident_list '=' expr ';'

return_stmt  <- 'return' ident_list ';'

ident_list   <- IDENT (',' IDENT)*

expr         <- or_expr

or_expr      <- xor_expr ('OR' xor_expr)*

xor_expr     <- and_expr ('XOR' and_expr)*

and_expr     <- unary ('AND' unary)*

unary        <- 'NOT' unary
              / primary

primary      <- call
              / IDENT
              / '(' expr ')'

call         <- IDENT '(' IDENT (',' IDENT)* ')'
              / IDENT '(' ')'

KEY          <- 'XOR' / 'OR' / 'AND' / 'NOT' / 'comp' / 'return'

IDENT        <- !KEY < [a-zA-Z_] [a-zA-Z0-9-]* >

%whitespace  <- [ \t\r\n]* ('//' (![\n] .)* [ \t\r\n]*)*
)";

} // namespace

std::optional<ast::Program> parse_program(std::string_view source,
                                          const char *path,
                                          std::string *error_out) {
  peg::parser peg_parser(kGrammar);
  if (!peg_parser) {
    if (error_out) {
      *error_out = "failed to compile prototype grammar";
    }
    return std::nullopt;
  }

  attach_actions(peg_parser);

  peg_parser.set_logger(
      [error_out](std::size_t line, std::size_t col, const std::string &msg,
                  const std::string & /*rule*/) {
        if (error_out) {
          *error_out =
              std::to_string(line) + ":" + std::to_string(col) + ": " + msg;
        }
      });

  ast::Program program;
  if (!peg_parser.parse_n(source.data(), source.size(), program, path)) {
    return std::nullopt;
  }
  return program;
}

} // namespace gate
