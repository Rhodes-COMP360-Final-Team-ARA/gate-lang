/**
 * File: GateoAliases.hpp
 * Purpose: Short names for gateo-cpp view types used by the compiler (single place to
 * retarget on a schema / view namespace bump).
 */
#pragma once

#include "gateo/v3/view.hpp"

namespace gate {

using GateObject = gateo::v3::view::GateObject;
using Node = gateo::v3::view::Node;
using GateType = gateo::v3::view::GateType;
using GateVersion = gateo::v3::view::Version;
using ComponentInstance = gateo::v3::view::ComponentInstance;

} // namespace gate
