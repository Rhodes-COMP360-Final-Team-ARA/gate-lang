/**
 * File: GateoAliases.hpp
 * Purpose: Short names for gateo-cpp view types used by the compiler (single place to
 * retarget on a schema / view namespace bump).
 */
#pragma once

#include "gateo/v2/view.hpp"

namespace gateo::v2::view {

inline bool operator==(Version const& a, Version const& b) noexcept {
  return a.major == b.major && a.minor == b.minor;
}

} // namespace gateo::v2::view

namespace gate {

using GateObject = gateo::v2::view::GateObject;
using GateVersion = gateo::v2::view::Version;
using ComponentInstance = gateo::v2::view::ComponentInstance;

} // namespace gate
