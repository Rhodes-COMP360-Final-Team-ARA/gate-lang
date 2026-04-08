/**
 * File: Simulation.cpp
 * Purpose: Circuit preparation (Kahn's algorithm) and gate evaluation loop.
 */
#include "Simulation.hpp"

#include <cassert>
#include <cstdint>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

namespace gate {

namespace {

std::string to_binary(uint64_t val, int width) {
  std::string result;
  result.reserve(width);
  for (int b = width - 1; b >= 0; --b) {
    result += ((val >> b) & 1) ? '1' : '0';
  }
  return result;
}

} // namespace

// ── prepare ─────────────────────────────────────────────────────────────────
/// Topologically sorts the circuit for evaluation. Until Kahn's algorithm is
/// implemented, returns an empty eval_order (simulate will produce zeros).
PreparedCircuit prepare(Circuit circuit) {
  // TODO: Kahn's algorithm.
  //
  // 1. Build in-degree count: in_degree[i] = nodes[i].inputs.size().
  // 2. Build consumers adjacency list: for each node, which nodes read from it?
  // 3. Seed queue with all nodes where in_degree == 0 (Input nodes).
  // 4. Process: dequeue → append to eval_order → decrement consumers' in_degree
  //    → enqueue any that reach zero.
  // 5. Assert eval_order.size() == nodes.size() (no cycles).

  PreparedCircuit pc;
  pc.circuit = std::move(circuit);
  return pc;
}

// ── simulate ────────────────────────────────────────────────────────────────
/// Evaluates the circuit against concrete input values. Walks eval_order,
/// computing each gate from its input nodes' values. Returns one uint64_t
/// per named output.
std::vector<uint64_t> simulate(const PreparedCircuit &pc,
                                const std::vector<uint64_t> &inputs) {
  const auto &nodes = pc.circuit.nodes;
  std::vector<uint64_t> values(nodes.size(), 0);

  for (size_t i = 0; i < pc.circuit.num_inputs; ++i) {
    uint64_t raw = (i < inputs.size()) ? inputs[i] : 0;
    uint64_t mask = (nodes[i].width >= 64) ? ~0ULL : (1ULL << nodes[i].width) - 1;
    values[i] = raw & mask;
  }

  for (size_t idx : pc.eval_order) {
    const Node &node = nodes[idx];
    if (node.type == GateType::Input) continue;

    uint64_t mask = (node.width >= 64) ? ~0ULL : (1ULL << node.width) - 1;

    switch (node.type) {
    case GateType::Not:
      values[idx] = ~values[node.inputs[0]] & mask;
      break;
    case GateType::And:
      values[idx] = (values[node.inputs[0]] & values[node.inputs[1]]) & mask;
      break;
    case GateType::Or:
      values[idx] = (values[node.inputs[0]] | values[node.inputs[1]]) & mask;
      break;
    case GateType::Xor:
      values[idx] = (values[node.inputs[0]] ^ values[node.inputs[1]]) & mask;
      break;
    case GateType::Input:
      break;
    }
  }

  std::vector<uint64_t> results;
  results.reserve(pc.circuit.outputs.size());
  for (const auto &out : pc.circuit.outputs) {
    results.push_back(values[out.signal.node]);
  }
  return results;
}

// ── format_outputs ──────────────────────────────────────────────────────────
/// Pairs each output name with its result value, formatted as decimal with
/// a binary representation zero-padded to the signal's width.
std::vector<std::string> format_outputs(const PreparedCircuit &pc,
                                         const std::vector<uint64_t> &results) {
  std::vector<std::string> lines;
  lines.reserve(pc.circuit.outputs.size());
  for (size_t i = 0; i < pc.circuit.outputs.size() && i < results.size(); ++i) {
    const auto &out = pc.circuit.outputs[i];
    std::ostringstream ss;
    ss << out.name << " = " << results[i] << " (0b" << to_binary(results[i], out.signal.width) << ")";
    lines.push_back(ss.str());
  }
  return lines;
}

} // namespace gate
