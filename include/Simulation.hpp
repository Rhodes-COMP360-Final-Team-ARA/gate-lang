/**
 * File: Simulation.hpp
 * Purpose: DAG evaluation — topological sort and gate simulation.
 *
 * Note: topological_sort() and operator() are declared as members of DAG
 * in DAG.hpp. This file is for any additional simulation utilities.
 */
#pragma once

#include "DAG.hpp"

#include <string>
#include <vector>

namespace gate {

// Format output values as "name = value" strings for REPL display.
std::vector<std::string> format_outputs(const DAG &dag,
                                        const std::vector<uint64_t> &results);

} // namespace gate
