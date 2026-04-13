/**
 * File: mutation.cpp
 * Purpose: Reassign an existing signal.
 */
#include "compiler/statement/stmt_mutation.hpp"
#include "compiler/CompileContext.hpp"
#include "compiler/CompileError.hpp"
#include "compiler/CompileExpr.hpp"

namespace gate {

void stmt_mutation(const ast::MutAssign &stmt, CompileContext &ctx) {
  const int expected_width = ctx.symtab.resolve(stmt.target).width;
  GateNodeRef sig = compile_expr(stmt.value, ctx);
  if (sig.width != expected_width)
    throw WidthMismatchError("assignment to '" + stmt.target + "'", expected_width,
                             sig.width);
  ctx.symtab.update(stmt.target, sig);
}

} // namespace gate
