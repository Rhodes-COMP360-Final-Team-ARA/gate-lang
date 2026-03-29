/**
 * File: Parser.hpp
 * Purpose: Parse GateLang prototype source into an `ast::Program`.
 */
#pragma once

#include "Ast.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace gate {

/**
 * Parse a prototype unit: **components only**, untyped identifier names.
 * @param source   Source text (UTF-8 as byte sequence; peglib treats it as bytes).
 * @param path     Optional logical path for diagnostics (may be nullptr).
 * @param error_out When non-null and parse fails, receives a short message.
 * @return Program on success, nullopt on failure.
 */
std::optional<ast::Program> parse_program(std::string_view source,
                                          const char *path = nullptr,
                                          std::string *error_out = nullptr);

} // namespace gate
