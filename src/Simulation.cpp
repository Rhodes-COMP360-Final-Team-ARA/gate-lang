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
  PreparedCircuit pc;
  pc.circuit = std::move(circuit);

  const auto &nodes = pc.circuit.nodes;
  const size_t n = nodes.size();

  // Kahn's algorithm:
  // - in_degree[v] counts remaining unmet dependencies for v.
  // - consumers[u] lists nodes that depend on u.
  std::vector<size_t> in_degree(n, 0);
  std::vector<std::vector<size_t>> consumers(n);

  for (size_t v = 0; v < n; ++v) {
    in_degree[v] = nodes[v].inputs.size();
    for (size_t u : nodes[v].inputs) {
      assert(u < n && "node input index out of bounds");
      consumers[u].push_back(v);
    }
  }

  std::queue<size_t> ready;
  for (size_t v = 0; v < n; ++v) {
    if (in_degree[v] == 0) ready.push(v);
  }

  pc.eval_order.clear();
  pc.eval_order.reserve(n);

  while (!ready.empty()) {
    const size_t u = ready.front();
    ready.pop();
    pc.eval_order.push_back(u);

    for (size_t v : consumers[u]) {
      assert(in_degree[v] > 0 && "in_degree underflow (corrupt graph?)");
      --in_degree[v];
      if (in_degree[v] == 0) ready.push(v);
    }
  }

  // If we couldn't process every node, the graph contains a cycle (or a node
  // depends on a missing input, which would have asserted above).
  assert(pc.eval_order.size() == n && "circuit contains a cycle");

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
