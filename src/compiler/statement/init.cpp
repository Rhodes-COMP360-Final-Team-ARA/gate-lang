/**
 * File: init.cpp
 * Purpose: Init assignment: declare a new signal from an expression.
 */
#include "compiler/statement/stmt_init.hpp"
#include "compiler/CompileContext.hpp"
#include "compiler/CompileError.hpp"
#include "compiler/CompileExpr.hpp"

namespace gate {

void stmt_init(const ast::InitAssign &stmt, CompileContext &ctx) {
  GateNodeRef sig = compile_expr(stmt.value, ctx);
  if (sig.width != stmt.target.width)
    throw WidthMismatchError("declaration of '" + stmt.target.ident + "'",
                             stmt.target.width, sig.width);
  ctx.symtab.define(stmt.target.ident, sig);
}

} // namespace gate
