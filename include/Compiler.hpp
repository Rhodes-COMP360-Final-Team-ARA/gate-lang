/**
 * File: Compiler.hpp
 * Purpose: AST → DAG compilation. Flattens component hierarchies into pure gate graphs.
 */
#pragma once

#include "Ast.hpp"
#include "DAG.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace gate {

using ComponentRegistry = std::unordered_map<std::string, ast::Comp>;
using SymbolTable = std::unordered_map<std::string, Signal>;

ComponentRegistry build_registry(const ast::Program &program);

std::optional<DAG> compile_component(const std::string &name,
                                     const ComponentRegistry &registry,
                                     std::string *error_out = nullptr);

} // namespace gate
