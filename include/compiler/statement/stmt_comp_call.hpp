#pragma once

#include "core/Ast.hpp"

namespace gate {

struct CompileContext;

void stmt_comp_call(const ast::CompCall &stmt, CompileContext &ctx);

} // namespace gate
