/**
 * File: comp_call.cpp
 * Purpose: Inline a callee GateObject into the caller and bind output signals.
 */
#include "compiler/statement/stmt_comp_call.hpp"
#include "compiler/CompileContext.hpp"
#include "compiler/CompileError.hpp"
#include "compiler/Compiler.hpp"
#include "compiler/GateCompileTypes.hpp"
#include "compiler/InlineGateo.hpp"

#include <string>
#include <vector>

namespace gate {

void stmt_comp_call(const ast::CompCall &stmt, CompileContext &ctx) {
  std::vector<GateNodeRef> args;
  args.reserve(stmt.args.size());
  for (const std::string &arg : stmt.args)
    args.push_back(ctx.symtab.resolve(arg));

  const ast::Comp &callee_ast = ctx.compiler->lookup_comp(stmt.comp);

  if (args.size() != callee_ast.params.size())
    throw ArityMismatchError("'" + stmt.comp + "' arguments",
                           callee_ast.params.size(), args.size());

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i].width != callee_ast.params[i].width)
      throw WidthMismatchError("'" + stmt.comp + "' argument '" +
                                   callee_ast.params[i].ident + "'",
                               callee_ast.params[i].width, args[i].width);
  }

  const GateObject &callee = ctx.compiler->get_comp(stmt.comp);

  std::vector<std::uint32_t> arg_nodes;
  arg_nodes.reserve(args.size());
  for (const GateNodeRef &a : args)
    arg_nodes.push_back(a.node_index);

  const InlineGateoResult inlined =
      inline_gateo_object(ctx.obj, callee, ctx.root_instance, arg_nodes);

  if (inlined.output_node_indices.size() != stmt.outputs.size())
    throw ArityMismatchError("'" + stmt.comp + "' outputs",
                           stmt.outputs.size(),
                           inlined.output_node_indices.size());

  for (size_t i = 0; i < stmt.outputs.size(); ++i) {
    const std::uint32_t out_idx = inlined.output_node_indices[i];
    const int w = static_cast<int>(ctx.obj.nodes[out_idx].width);
    if (stmt.outputs[i].width != w)
      throw WidthMismatchError("'" + stmt.comp + "' output '" +
                                   stmt.outputs[i].ident + "'",
                               w, stmt.outputs[i].width);
    ctx.symtab.define(stmt.outputs[i].ident,
                      GateNodeRef{out_idx, stmt.outputs[i].width});
  }
}

} // namespace gate
