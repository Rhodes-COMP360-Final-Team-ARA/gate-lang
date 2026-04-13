#pragma once

#include "core/Ast.hpp"

namespace gate {

struct CompileContext;

void stmt_return(const ast::ReturnStmt &stmt, CompileContext &ctx);

} // namespace gate
