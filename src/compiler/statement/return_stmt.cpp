/**
 * File: return_stmt.cpp
 * Purpose: Emit Output boundary nodes for each returned signal.
 */
#include "compiler/statement/stmt_return.hpp"
#include "compiler/CompileContext.hpp"
#include "compiler/GateCompileTypes.hpp"
#include "compiler/CompileOps.hpp"

#include "gateo/v2/view.hpp"

#include <string>

namespace gate {

void stmt_return(const ast::ReturnStmt &stmt, CompileContext &ctx) {
  using gateo::v2::view::GateType;

  for (const std::string &name : stmt.names) {
    const GateNodeRef sig = ctx.symtab.resolve(name);
    (void)append_node(ctx, GateType::Output, {sig.node_index}, sig.width, name);
  }
  ctx.returned = true;
}

} // namespace gate
