#ifndef SOLITAIRE_UTIL_MOVE_NOTATION_H
#define SOLITAIRE_UTIL_MOVE_NOTATION_H

// Human-reviewed: do not edit further with AI assistance unless explicitly confirmed.

#include "../types.h"
#include <optional>
#include <string>
#include <string_view>

namespace solitaire::util {

// Generate human-readable notation for a move (e.g., "T0→F2", "Draw", "W→T1")
std::string move_to_notation(const Move& move);

// Parse notation produced by move_to_notation(); returns std::nullopt on failure.
std::optional<Move> move_from_notation(std::string_view notation);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_MOVE_NOTATION_H
