// Human-reviewed: do not edit further with AI assistance unless explicitly confirmed.
#include "solitaire/types.h"
#include "solitaire/util/move_notation.h"
#include <array>
#include <sstream>

namespace solitaire {

std::string to_string(SolverStatus status) {
    switch (status) {
        case SolverStatus::Success:
            return "Success";
        case SolverStatus::ReachedMaxDepth:
            return "MaxDepth";
        case SolverStatus::ReachedMaxNodes:
            return "MaxNodes";
        case SolverStatus::InvalidState:
            return "Invalid";
        default:
            return "Unknown";
    }
}

std::string Card::to_string() const {
    constexpr std::array<const char*, 14> rank_chars = {{
        "?", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
    }};
    constexpr std::array<const char*, 4> suit_chars = {{"♥", "♦", "♣", "♠"}};

    return std::string(rank_chars[static_cast<int>(rank())]) +
           suit_chars[static_cast<int>(suit())];
}

std::string PileId::to_string() const {
    constexpr std::array<const char*, 4> kind_chars = {{"T", "F", "S", "W"}};
    std::string result(kind_chars[static_cast<int>(kind())]);
    // Append index only for Tableau and Foundation, which are kinds 0 and 1.
    result += (static_cast<int>(kind()) < 2) ? std::to_string(index()) : "";
    return result;
}

std::string SolverResult::to_string() const {
    std::stringstream ss;
    ss << "SolverResult: solvable=" << solvable
       << ", nodes_explored=" << stats.nodes_explored
       << ", time_seconds=" << stats.time_seconds
       << ", solution_length=" << solution.size();
    return ss.str();
}

bool Move::is_valid() const {
    const auto is_tableau = [](int idx) {
        return idx >= 0 && idx < NUM_TABLEAU_PILES;
    };
    const auto is_foundation = [](int idx) {
        return idx >= 0 && idx < NUM_FOUNDATIONS;
    };

    const int src = source.index();
    const int tgt = target.index();

    switch (kind) {
        case MoveKind::TableauToTableau:
            return source.kind() == PileKind::Tableau &&
                   target.kind() == PileKind::Tableau &&
                   is_tableau(src) &&
                   is_tableau(tgt) &&
                   src != tgt &&
                   card_count >= 1;

        case MoveKind::TableauToFoundation:
            return source.kind() == PileKind::Tableau &&
                   target.kind() == PileKind::Foundation &&
                   is_tableau(src) &&
                   is_foundation(tgt) &&
                   card_count == 1;

        case MoveKind::WasteToTableau:
            return source.kind() == PileKind::Waste &&
                   source.index() == 0 &&
                   target.kind() == PileKind::Tableau &&
                   is_tableau(tgt) &&
                   card_count == 1;

        case MoveKind::WasteToFoundation:
            return source.kind() == PileKind::Waste &&
                   source.index() == 0 &&
                   target.kind() == PileKind::Foundation &&
                   is_foundation(tgt) &&
                   card_count == 1;

        case MoveKind::StockDraw:
            return source == PileId(PileKind::Stock, 0) &&
                   target == PileId(PileKind::Waste, 0) &&
                   card_count >= 1;

        case MoveKind::StockRecycle:
            return source == PileId(PileKind::Waste, 0) &&
                   target == PileId(PileKind::Stock, 0);    
    }

    return false;
}

std::string Move::to_string() const {
    // Lazy-generate notation on demand via util module
    return util::move_to_notation(*this);
}

}  // namespace solitaire
