#include "compiler/CompCache.hpp"
#include "compiler/CompileComponent.hpp"
#include "compiler/CompileError.hpp"

namespace gate {

CompCache::CompCache(const std::vector<ast::Comp> &program_comps) {
  for (const ast::Comp &c : program_comps)
    ast_comps_[c.name] = &c;
}

const GateObject &CompCache::resolve(const std::string &name) {
  // Check cache for already compiled
  if (auto it = cache_.find(name); it != cache_.end())
    return it->second;

  // TODO: Check filesystem for pre-built .gateo file.

  // Find it in the AST and compile it
  auto ast_it = ast_comps_.find(name);
  if (ast_it == ast_comps_.end())
    throw UndefinedComponentError(name);

  if (in_progress_.count(name))
    throw CyclicDependencyError(name);

  in_progress_.insert(name);
  cache_.emplace(name, compile_component(*ast_it->second, *this));
  in_progress_.erase(name);

  return cache_.at(name);
}

} // namespace gate
