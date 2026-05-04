#include "solitaire/util/move_analysis.h"
#include "solitaire/util/board_equivalence.h"

#include <algorithm>
#include <optional>

namespace {

bool move_less(const solitaire::Move& a, const solitaire::Move& b) {
    if (a.kind != b.kind) return a.kind < b.kind;
    if (a.source.raw_data() != b.source.raw_data()) return a.source.raw_data() < b.source.raw_data();
    if (a.target.raw_data() != b.target.raw_data()) return a.target.raw_data() < b.target.raw_data();
    return a.card_count < b.card_count;
}

bool move_equal(const solitaire::Move& a, const solitaire::Move& b) {
    return a.kind == b.kind &&
           a.source == b.source &&
           a.target == b.target &&
           a.card_count == b.card_count;
}

bool no_new_moves_after_move(const solitaire::GameState& before,
    const solitaire::Move& move,
    const solitaire::GameState& after);

std::vector<solitaire::Move> normalize_moves(const solitaire::MoveList& moves) {
    std::vector<solitaire::Move> keys;
    keys.reserve(moves.size());
    for (const auto& move : moves) {
        keys.push_back(move);
    }
    std::sort(keys.begin(), keys.end(), move_less);
    return keys;
}

bool same_move_set(const solitaire::MoveList& a, const solitaire::MoveList& b) {
    const auto normalized_a = normalize_moves(a);
    const auto normalized_b = normalize_moves(b);

    if (normalized_a.size() != normalized_b.size()) {
        return false;
    }

    return std::equal(normalized_a.begin(), normalized_a.end(), normalized_b.begin(), move_equal);
}

bool is_waste_origin_move(const solitaire::Move& move) {
    return move.kind == solitaire::MoveKind::WasteToTableau ||
           move.kind == solitaire::MoveKind::WasteToFoundation;
}

bool has_waste_origin_moves(const solitaire::MoveList& moves) {
    return std::any_of(moves.begin(), moves.end(), [](const solitaire::Move& move) {
        return is_waste_origin_move(move);
    });
}

std::optional<solitaire::Move> inverse_if_reversible(const solitaire::Move& move) {
    if (move.kind != solitaire::MoveKind::TableauToTableau) {
        return std::nullopt;
    }

    return solitaire::Move(
        solitaire::PileId(solitaire::PileKind::Tableau, move.target.index()),
        solitaire::PileId(solitaire::PileKind::Tableau, move.source.index()),
        solitaire::MoveKind::TableauToTableau,
        move.card_count
    );
}

bool makes_progress(const solitaire::GameState& before,
                    const solitaire::Move& move,
                    const solitaire::GameState& after) {
    switch (move.kind) {
        case solitaire::MoveKind::TableauToTableau:
            for (int pile = 0; pile < solitaire::NUM_TABLEAU_PILES; ++pile) {
                if (after.tableau_face_down_count(pile) < before.tableau_face_down_count(pile)) {
                    return true;
                }
            }
            return false;

        case solitaire::MoveKind::TableauToFoundation:
        case solitaire::MoveKind::WasteToTableau:
        case solitaire::MoveKind::WasteToFoundation:
            return true;

        case solitaire::MoveKind::StockDraw:
        case solitaire::MoveKind::StockRecycle:
            return false;
    }

    return false;
}

bool is_non_stock_no_op(const solitaire::GameState& state, const solitaire::Move& move, const solitaire::GameState& next) {
    // If the move makes progress, it is not a no-op
    if (makes_progress(state, move, next)) {
        return false;
    }

    // If the move doesn't make progress,
    // Then we check whether it reveals other possible moves
    return no_new_moves_after_move(state, move, next);
}

bool is_stock_cycle_no_op(const solitaire::GameState& original_state,
                          const solitaire::GameState& start_after_move) {
    const solitaire::GameState canonical_original = solitaire::util::canonicalize_board_state(original_state);
    solitaire::GameState current = solitaire::util::canonicalize_board_state(start_after_move);
    std::vector<solitaire::GameState> seen_positions;
    seen_positions.push_back(current);

    while (true) {
        auto legal = current.legal_moves();
        if (has_waste_origin_moves(legal)) {
            return false;
        }

        if (current.stock_size() > 0) {
            solitaire::Move draw(
                solitaire::PileId(solitaire::PileKind::Stock, 0),
                solitaire::PileId(solitaire::PileKind::Waste, 0),
                solitaire::MoveKind::StockDraw,
                current.config().cards_per_draw);
            if (!current.is_legal(draw)) {
                return false;
            }
            current = current.apply_move(draw);
            continue;
        }

        if (current.waste_size() == 0 || !current.config().unlimited_recycle) {
            return false;
        }

        solitaire::Move recycle(
            solitaire::PileId(solitaire::PileKind::Waste, 0),
            solitaire::PileId(solitaire::PileKind::Stock, 0),
            solitaire::MoveKind::StockRecycle,
            current.waste_size());
        if (!current.is_legal(recycle)) {
            return false;
        }

        current = current.apply_move(recycle);
        current = solitaire::util::canonicalize_board_state(current);

        if (current.same_position(canonical_original)) {
            return true;
        }

        for (const auto& seen : seen_positions) {
            if (current.same_position(seen)) {
                return true;
            }
        }
        seen_positions.push_back(current);
    }
}

bool no_new_moves_after_move(const solitaire::GameState& before,
                             const solitaire::Move& move,
                             const solitaire::GameState& after) {
    const solitaire::GameState canonical_before = solitaire::util::canonicalize_board_state(before);
    const solitaire::GameState canonical_after = solitaire::util::canonicalize_board_state(after);

    auto before_moves = canonical_before.legal_moves();
    auto after_moves = canonical_after.legal_moves();

    // Remove the considered move from the pre-move move list.
    auto expected = before_moves;
    auto before_it = std::find_if(expected.begin(), expected.end(), [&](const solitaire::Move& candidate) {
        return candidate.source == move.source &&
               candidate.target == move.target &&
               candidate.kind == move.kind &&
               candidate.card_count == move.card_count;
    });
    if (before_it != expected.end()) {
        expected.erase(before_it);
    }

    if (same_move_set(expected, after_moves)) {
        return true;
    }

    auto inverse = inverse_if_reversible(move);
    if (inverse) {
        auto expected_with_inverse = expected;
        expected_with_inverse.push_back(*inverse);
        if (same_move_set(expected_with_inverse, after_moves)) {
            return true;
        }
    }

    return false;
}

}  // namespace

namespace solitaire::util {

bool is_no_op_move(const GameState& state, const Move& move) {
    // If the move is illegal, it's not a no-op.
    if (!state.is_legal(move)) {
        return false;
    }

    GameState next = state.apply_move(move);

    // If the move leaves the position unchanged, it's a no-op.
    if (next == state) {
        return true;
    }


    // If the move is a stock draw/recycle then we need a more complicated check...
    if (move.kind == MoveKind::StockDraw || move.kind == MoveKind::StockRecycle) {
        return is_stock_cycle_no_op(state, next);
    } else {
        return is_non_stock_no_op(state, move, next);
    }
}

}  // namespace solitaire::util
