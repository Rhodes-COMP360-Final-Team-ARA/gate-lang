/**
 * File: Compiler.cpp
 * Purpose: AST → DAG compilation with semantic validation.
 */
#include "Compiler.hpp"

#include <variant>

namespace gate {

namespace {

// ── Error helper ────────────────────────────────────────────────────────────

bool report(std::string *out, const std::string &msg) {
  if (out) *out = msg;
  return false;
}

// ── Expression compilation ──────────────────────────────────────────────────

// Recursively compile an expression into the DAG.
// Returns the Signal (node index + width) for the expression's result.
// On error, writes to error_out and returns std::nullopt.
std::optional<Signal> compile_expr(const ast::Expr &expr, DAG &dag,
                                   const SymbolTable &symtab,
                                   std::string *error_out) {
  // TODO: visit expr.data
  //
  // string (identifier):
  //   Look up in symtab. Error if not found. Return its Signal as-is.
  //
  // UnaryExpr (NOT):
  //   Compile operand recursively.
  //   Create a NOT node: dag.add_node(GateType::Not, {operand.node}, operand.width)
  //   Return Signal{new_index, operand.width}.
  //
  // BinExpr (AND/OR/XOR):
  //   Compile LHS and RHS recursively.
  //   Check lhs.width == rhs.width — error on mismatch.
  //   Map ast::BinOp to GateType.
  //   Create the gate node.
  //   Return Signal{new_index, lhs.width}.

  (void)expr;
  (void)dag;
  (void)symtab;
  (void)error_out;
  return std::nullopt;
}

// ── Body compilation ────────────────────────────────────────────────────────

// Process one component's body, adding gates into `dag`.
// `arg_nodes` maps each formal parameter (by position) to an existing node
// index in the DAG — either an Input node (top-level) or a caller's node
// (inlined call).
// Returns the node indices for the component's return values.
std::optional<std::vector<size_t>>
compile_body(const ast::Comp &comp, const std::vector<size_t> &arg_nodes,
             DAG &dag, const ComponentRegistry &registry,
             std::string *error_out) {
  SymbolTable symtab;

  // ── Bind formal parameters to argument nodes ──
  // TODO: For each param in comp.params, map param.ident → Signal{arg_nodes[i], param.width}
  //       in the symbol table. Validate arg_nodes.size() == comp.params.size().

  std::vector<size_t> return_nodes;

  for (const auto &stmt : comp.body) {
    // TODO: Use std::visit to dispatch on the Statement variant.
    //
    // InitAssign:
    //   auto sig = compile_expr(stmt.value, dag, symtab, error_out);
    //   Check sig->width == stmt.target.width.
    //   Insert stmt.target.ident → *sig into symtab.
    //   Error if name already exists.
    //
    // MutAssign:
    //   Look up stmt.target in symtab to get existing width.
    //   auto sig = compile_expr(stmt.value, dag, symtab, error_out);
    //   Check widths match.
    //   Update symtab entry to point to new node.
    //
    // CompCall:
    //   Look up stmt.comp in registry — error if not found.
    //   Resolve each arg name in symtab to get node indices.
    //   Validate arg count and widths against the called comp's params.
    //   Call compile_body recursively with the sub-comp's AST and arg node indices.
    //   Bind each output name (stmt.outputs[i].ident) to the returned node indices.
    //
    // ReturnStmt:
    //   For each name, look up in symtab and push node index to return_nodes.

    (void)stmt;
  }

  return return_nodes;
}

} // namespace

// ── Public API ──────────────────────────────────────────────────────────────

ComponentRegistry build_registry(const ast::Program &program) {
  ComponentRegistry reg;
  for (const auto &comp : program.components) {
    reg[comp.name] = comp;
  }
  return reg;
}

std::optional<DAG> compile_component(const std::string &name,
                                     const ComponentRegistry &registry,
                                     std::string *error_out) {
  auto it = registry.find(name);
  if (it == registry.end()) {
    report(error_out, "undefined component: " + name);
    return std::nullopt;
  }

  const ast::Comp &comp = it->second;
  DAG dag;

  // Create input nodes (first n nodes in the DAG, by convention).
  std::vector<size_t> input_nodes;
  for (const auto &param : comp.params) {
    size_t idx = dag.add_node(GateType::Input, {}, param.width);
    input_nodes.push_back(idx);
  }
  dag.num_inputs = input_nodes.size();

  auto ret = compile_body(comp, input_nodes, dag, registry, error_out);
  if (!ret) return std::nullopt;

  // Populate named outputs from the return statement.
  // compile_body fills return_nodes; pair them with names from the comp's
  // return statement (the last statement in body should be ReturnStmt).
  // NOTE: compile_body already built dag.outputs via ReturnStmt handling,
  // OR you can pair them here — your choice on where to put this logic.

  dag.topological_sort();
  return dag;
}

} // namespace gate
