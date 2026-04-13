#pragma once

#include "core/Ast.hpp"

namespace gate {

struct CompileContext;

void stmt_init(const ast::InitAssign &stmt, CompileContext &ctx);

} // namespace gate
