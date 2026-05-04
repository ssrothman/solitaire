#include "solitaire/util/move_analysis.h"

#include <algorithm>
#include <optional>

namespace {

struct MoveKey {
    solitaire::MoveKind kind;
    solitaire::PileId source;
    solitaire::PileId target;
    int card_count;

    bool operator<(const MoveKey& other) const {
        if (kind != other.kind) return kind < other.kind;
        if (source.raw_data() != other.source.raw_data()) return source.raw_data() < other.source.raw_data();
        if (target.raw_data() != other.target.raw_data()) return target.raw_data() < other.target.raw_data();
        return card_count < other.card_count;
    }

    bool operator==(const MoveKey& other) const {
        return kind == other.kind && source == other.source && target == other.target && card_count == other.card_count;
    }
};

MoveKey to_key(const solitaire::Move& move) {
    return {move.kind, move.source, move.target, move.card_count};
}

std::vector<MoveKey> normalize_moves(const solitaire::MoveList& moves) {
    std::vector<MoveKey> keys;
    keys.reserve(moves.size());
    for (const auto& move : moves) {
        keys.push_back(to_key(move));
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

bool same_move_set(const solitaire::MoveList& a, const solitaire::MoveList& b) {
    return normalize_moves(a) == normalize_moves(b);
}

bool is_non_stock_move(const solitaire::Move& move) {
    return move.kind != solitaire::MoveKind::StockDraw && move.kind != solitaire::MoveKind::StockRecycle;
}

bool has_non_stock_moves(const solitaire::MoveList& moves) {
    return std::any_of(moves.begin(), moves.end(), [](const solitaire::Move& move) {
        return is_non_stock_move(move);
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
        move.card_count);
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

bool is_stock_cycle_no_op(const solitaire::GameState& original_state,
                          const solitaire::GameState& start_after_move) {
    solitaire::GameState current = start_after_move;
    std::vector<solitaire::GameState> seen_positions;
    seen_positions.push_back(current);

    while (true) {
        auto legal = current.legal_moves();
        if (has_non_stock_moves(legal)) {
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
        if (current.same_position(original_state)) {
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
    auto before_moves = before.legal_moves();
    auto after_moves = after.legal_moves();

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
    if (!state.is_legal(move)) {
        return false;
    }

    GameState next = state.apply_move(move);
    if (next == state) {
        return true;
    }

    if (makes_progress(state, move, next)) {
        return false;
    }

    if (move.kind == MoveKind::StockDraw || move.kind == MoveKind::StockRecycle) {
        return is_stock_cycle_no_op(state, next);
    }

    return no_new_moves_after_move(state, move, next);
}

}  // namespace solitaire::util
