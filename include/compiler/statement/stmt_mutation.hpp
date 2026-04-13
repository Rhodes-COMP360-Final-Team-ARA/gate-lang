#pragma once

#include "core/Ast.hpp"

namespace gate {

struct CompileContext;

void stmt_mutation(const ast::MutAssign &stmt, CompileContext &ctx);

} // namespace gate
