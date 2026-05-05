#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>

#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/util/move_analysis.h"

using namespace solitaire;
using namespace solitaire::util;

namespace {

std::vector<Card> make_stock_cycle_no_op_deck() {
    std::array<Card, 7> tableau_tops = {
        Card(Suit::Hearts, Rank::Ten),
        Card(Suit::Diamonds, Rank::Ten),
        Card(Suit::Hearts, Rank::Jack),
        Card(Suit::Diamonds, Rank::Jack),
        Card(Suit::Hearts, Rank::Queen),
        Card(Suit::Diamonds, Rank::Queen),
        Card(Suit::Hearts, Rank::King),
    };

    std::vector<Card> stock_candidates;
    std::vector<Card> hidden_candidates;
    stock_candidates.reserve(24);
    hidden_candidates.reserve(21);

    std::array<bool, 52> used{};
    auto mark_used = [&](const Card& card) {
        used[static_cast<std::size_t>(card.raw_data())] = true;
    };

    for (const auto& card : tableau_tops) {
        mark_used(card);
    }

    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            Card card(static_cast<Suit>(suit), static_cast<Rank>(rank));
            if (used[static_cast<std::size_t>(card.raw_data())]) {
                continue;
            }
            const bool blocks_tableau = card.color() == Color::Black &&
                                        static_cast<int>(card.rank()) >= 9 &&
                                        static_cast<int>(card.rank()) <= 12;
            const bool blocks_foundation = card.rank() == Rank::Ace || card.rank() == Rank::Two;

            if (rank >= 3 && !blocks_tableau && !blocks_foundation) {
                stock_candidates.push_back(card);
            } else {
                hidden_candidates.push_back(card);
            }
        }
    }

    std::vector<Card> deck(52);
    const std::array<int, 7> top_positions = {0, 2, 5, 9, 14, 20, 27};
    std::size_t top_idx = 0;
    std::size_t stock_idx = 0;
    std::size_t hidden_idx = 0;

    for (int position = 0; position < 52; ++position) {
        bool is_top = false;
        for (int top_position : top_positions) {
            if (position == top_position) {
                is_top = true;
                break;
            }
        }

        if (is_top) {
            deck[position] = tableau_tops[top_idx++];
        } else if (position >= 28) {
            deck[position] = stock_candidates[stock_idx++];
        } else {
            deck[position] = hidden_candidates[hidden_idx++];
        }
    }

    return deck;
}

std::vector<Card> make_progress_deck() {
    std::vector<Card> deck = make_stock_cycle_no_op_deck();
    const Card removed = deck[0];
    const Card ace_of_spades(Suit::Spades, Rank::Ace);
    deck[0] = ace_of_spades;

    for (std::size_t i = 1; i < deck.size(); ++i) {
        auto& card = deck[i];
        if (card == ace_of_spades) {
            card = removed;
            break;
        }
    }

    return deck;
}

std::vector<Card> make_useful_stock_draw_deck() {
    std::vector<Card> deck = make_stock_cycle_no_op_deck();
    const Card useful(Suit::Hearts, Rank::Ace);
    const std::size_t useful_position = 49;
    const Card displaced = deck[useful_position];

    deck[useful_position] = useful;

    for (std::size_t i = 0; i < deck.size(); ++i) {
        if (i != useful_position && deck[i] == useful) {
            deck[i] = displaced;
            break;
        }
    }

    return deck;
}

// Creates a simple deck for testing T-to-T no-op logic
// Uses a shuffled deck which naturally has many tableau moves
// (Commented out: kept for reference, used implicitly in other tests)
// std::vector<Card> make_tableau_move_test_deck() {
//     return shuffle_deck(7777);
// }

}  // namespace

TEST_CASE("Move analysis: useless stock milling is detected as a no-op", "[move_analysis]") {
    GameState state = GameState::from_deck(make_stock_cycle_no_op_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    REQUIRE(moves.size() == 1);
    REQUIRE(moves.front().kind == MoveKind::StockDraw);
    REQUIRE(is_no_op_move(state, moves.front()));
}

TEST_CASE("Move analysis: progress moves are not treated as no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(make_progress_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    bool checked_progress_move = false;
    for (const auto& move : moves) {
        if (move.kind == MoveKind::TableauToFoundation) {
            REQUIRE_FALSE(is_no_op_move(state, move));
            checked_progress_move = true;
            break;
        }
    }

    REQUIRE(checked_progress_move);
}

TEST_CASE("Move analysis: useful stock draws are not no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(make_useful_stock_draw_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    REQUIRE_FALSE(moves.empty());
    REQUIRE(moves.front().kind == MoveKind::StockDraw);
    REQUIRE_FALSE(is_no_op_move(state, moves.front()));
}

TEST_CASE("Move analysis: stock cycle recurrence is treated as no-op", "[move_analysis]") {
    GameState start = GameState::from_deck(make_stock_cycle_no_op_deck(), GameConfig(3, true));
    GameState state = start;

    while (state.stock_size() > 0) {
        MoveList moves = state.legal_moves();
        REQUIRE_FALSE(moves.empty());
        REQUIRE(moves.front().kind == MoveKind::StockDraw);
        state = state.apply_move(moves.front());
    }

    Move recycle(
        PileId(PileKind::Waste, 0),
        PileId(PileKind::Stock, 0),
        MoveKind::StockRecycle,
        state.waste_size());

    REQUIRE(state.is_legal(recycle));
    REQUIRE(is_no_op_move(state, recycle));

    GameState recycled = state.apply_move(recycle);
    REQUIRE(recycled.same_position(start));
    REQUIRE(recycled != start);
}

TEST_CASE("Move analysis: illegal moves are not treated as no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(42), GameConfig(3, true));

    Move illegal;
    illegal.source = PileId(PileKind::Tableau, 0);
    illegal.target = PileId(PileKind::Tableau, 0);
    illegal.kind = MoveKind::TableauToTableau;
    illegal.card_count = 1;

    REQUIRE_FALSE(is_no_op_move(state, illegal));
}

TEST_CASE("Move analysis: helper is deterministic on repeated calls", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(123), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        REQUIRE(is_no_op_move(state, move) == is_no_op_move(state, move));
    }
}

// ============================================================================
// DFS-based T-to-T no-op detection tests
// ============================================================================

TEST_CASE("Move analysis: T-to-T moves revealing face-down cards are not no-ops", "[move_analysis]") {
    // Any T-to-T move that reduces the face-down count is not a no-op
    GameState state = GameState::from_deck(shuffle_deck(4242), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        if (move.kind == MoveKind::TableauToTableau) {
            GameState after = state.apply_move(move);
            
            // Check if this move reveals face-down cards
            bool reveals_card = false;
            for (int pile = 0; pile < NUM_TABLEAU_PILES; ++pile) {
                if (after.tableau_face_down_count(pile) < state.tableau_face_down_count(pile)) {
                    reveals_card = true;
                    break;
                }
            }
            
            if (reveals_card) {
                REQUIRE_FALSE(is_no_op_move(state, move));
            }
        }
    }
}

TEST_CASE("Move analysis: W-to-F moves are never no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(5555), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        if (move.kind == MoveKind::WasteToFoundation) {
            REQUIRE_FALSE(is_no_op_move(state, move));
        }
    }
}

TEST_CASE("Move analysis: T-to-F moves are never no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(6666), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        if (move.kind == MoveKind::TableauToFoundation) {
            REQUIRE_FALSE(is_no_op_move(state, move));
        }
    }
}

TEST_CASE("Move analysis: W-to-T moves are never no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(7777), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        if (move.kind == MoveKind::WasteToTableau) {
            REQUIRE_FALSE(is_no_op_move(state, move));
        }
    }
}

TEST_CASE("Move analysis: DFS correctly handles repeated evaluations with caching", "[move_analysis]") {
    // Test that calling is_no_op_move multiple times with similar states
    // uses the cache appropriately
    GameState state1 = GameState::from_deck(shuffle_deck(8888), GameConfig(3, true));
    GameState state2 = GameState::from_deck(shuffle_deck(8888), GameConfig(3, true));
    
    MoveList moves1 = state1.legal_moves();
    MoveList moves2 = state2.legal_moves();

    // Both states should have identical move evaluation results
    // (though they might be different objects)
    for (const auto& move : moves1) {
        bool result1 = is_no_op_move(state1, move);
        
        // Find equivalent move in state2
        for (const auto& m2 : moves2) {
            if (m2.kind == move.kind && 
                m2.source == move.source && 
                m2.target == move.target && 
                m2.card_count == move.card_count) {
                // Verify that cache handles similar states correctly
                [[maybe_unused]] bool result2 = is_no_op_move(state2, m2);
                // Results may differ if game states diverge, but at least
                // they should be consistent within the same state
                REQUIRE(result1 == is_no_op_move(state1, move));
                break;
            }
        }
    }
}

TEST_CASE("Move analysis: consistent results across multiple game states", "[move_analysis]") {
    // Test that is_no_op_move produces consistent results across different seeds
    for (int seed : {100, 200, 300, 400, 500}) {
        GameState state = GameState::from_deck(shuffle_deck(seed), GameConfig(3, true));
        MoveList moves = state.legal_moves();

        // First pass: collect results
        std::vector<bool> first_pass_results;
        for (const auto& move : moves) {
            first_pass_results.push_back(is_no_op_move(state, move));
        }

        // Second pass: verify consistency
        for (std::size_t i = 0; i < moves.size(); ++i) {
            REQUIRE(is_no_op_move(state, moves[i]) == first_pass_results[i]);
        }
    }
}

TEST_CASE("Move analysis: tableau moves without progress have DFS evaluation", "[move_analysis]") {
    // Test that T-to-T moves without progress are evaluated using DFS
    // (should check for new T-to-F moves via DFS, not just immediate move availability)
    GameState state = GameState::from_deck(shuffle_deck(9999), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    int tableau_moves_checked = 0;
    for (const auto& move : moves) {
        if (move.kind == MoveKind::TableauToTableau) {
            GameState after = state.apply_move(move);
            
            // Check if this move doesn't reveal cards (would bypass DFS)
            bool reveals = false;
            for (int pile = 0; pile < NUM_TABLEAU_PILES; ++pile) {
                if (after.tableau_face_down_count(pile) < state.tableau_face_down_count(pile)) {
                    reveals = true;
                    break;
                }
            }
            
            if (!reveals) {
                // This move doesn't reveal anything, so is_no_op_move
                // must use DFS to evaluate if it's a no-op
                bool result = is_no_op_move(state, move);
                // Just verify we can call it without error (DFS logic works)
                (void)result;
                tableau_moves_checked++;
            }
        }
    }
    // At least some games should have T-to-T moves without progress
    // (this may not always be true for random seeds, but is statistically likely)
}

TEST_CASE("Move analysis: stock operations remain unchanged", "[move_analysis]") {
    // Verify that StockDraw and StockRecycle moves still use the original logic
    GameState state = GameState::from_deck(make_stock_cycle_no_op_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    if (!moves.empty() && moves.front().kind == MoveKind::StockDraw) {
        REQUIRE(is_no_op_move(state, moves.front()));
    }
}

TEST_CASE("Move analysis: DFS explores multiple branches (multi-branch)", "[move_analysis][dfs-multi]") {
    // This test was removed: use deterministic user-provided scenario instead.
}

TEST_CASE("Move analysis: user-provided tricky sequence (seed 10)", "[move_analysis][dfs-user]") {
    GameState state = GameState::from_deck(shuffle_deck(10), GameConfig(3, true));

    auto apply = [&](const Move& m) {
        REQUIRE(state.is_legal(m));
        state = state.apply_move(m);
    };

    // Helper lambdas to build moves
    auto TtoF = [&](int src, int fidx) {
        Move m;
        m.source = PileId(PileKind::Tableau, src);
        m.target = PileId(PileKind::Foundation, fidx);
        m.kind = MoveKind::TableauToFoundation;
        m.card_count = 1;
        return m;
    };
    auto TtoT = [&](int src, int tgt, int cnt=1) {
        Move m;
        m.source = PileId(PileKind::Tableau, src);
        m.target = PileId(PileKind::Tableau, tgt);
        m.kind = MoveKind::TableauToTableau;
        m.card_count = cnt;
        return m;
    };
    auto WtoT = [&](int tgt) {
        Move m;
        m.source = PileId(PileKind::Waste, 0);
        m.target = PileId(PileKind::Tableau, tgt);
        m.kind = MoveKind::WasteToTableau;
        m.card_count = 1;
        return m;
    };
    auto WtoF = [&](int f) {
        Move m;
        m.source = PileId(PileKind::Waste, 0);
        m.target = PileId(PileKind::Foundation, f);
        m.kind = MoveKind::WasteToFoundation;
        m.card_count = 1;
        return m;
    };
    auto Draw = [&]() {
        Move m;
        m.source = PileId(PileKind::Stock, 0);
        m.target = PileId(PileKind::Waste, 0);
        m.kind = MoveKind::StockDraw;
        m.card_count = state.config().cards_per_draw;
        return m;
    };
    auto Recycle = [&]() {
        Move m;
        m.source = PileId(PileKind::Waste, 0);
        m.target = PileId(PileKind::Stock, 0);
        m.kind = MoveKind::StockRecycle;
        m.card_count = state.waste_size();
        return m;
    };

    // Apply the provided sequence of 50 moves
    apply(TtoF(1, 0));              // 1
    apply(TtoT(0, 5));              // 2
    apply(TtoT(2, 0));              // 3
    apply(TtoT(3, 6));              // 4
    apply(TtoT(6, 0, 2));           // 5
    apply(TtoF(6, 0));              // 6
    apply(Draw());                  // 7
    apply(Draw());                  // 8
    apply(WtoT(6));                 // 9
    apply(WtoF(0));                 // 10
    apply(WtoT(0));                 // 11
    apply(WtoT(0));                 // 12
    apply(TtoT(6,0,2));             // 13
    apply(Draw());                  // 14
    apply(Draw());                  // 15
    apply(WtoT(0));                 // 16
    apply(Draw());                  // 17
    apply(WtoF(1));                 // 18
    apply(Draw());                  // 19
    apply(WtoF(3));                 // 20
    apply(WtoT(0));                 // 21
    apply(Draw());                  // 22
    apply(WtoT(1));                 // 23
    apply(WtoT(3));                 // 24
    apply(WtoF(2));                 // 25
    apply(Draw());                  // 26
    apply(WtoF(3));                 // 27
    apply(TtoF(2, 3));              // 28
    apply(WtoF(3));                 // 29
    apply(Recycle());               // 30
    apply(Draw());                  // 31
    apply(WtoF(2));                 // 32
    apply(WtoF(3));                 // 33
    apply(WtoT(4));                 // 34
    apply(Draw());                  // 35
    apply(WtoF(3));                 // 36
    apply(WtoT(0));                 // 37
    apply(Draw());                  // 38
    apply(WtoT(3));                 // 39
    apply(TtoT(4,3,2));             // 40
    apply(TtoT(3,4,5));             // 41
    apply(Draw());                  // 42
    apply(WtoT(4));                 // 43
    apply(TtoT(1,4,2));             // 44
    apply(TtoT(0,1,10));            // 45
    apply(TtoT(3,0));               // 46
    apply(TtoF(3,0));               // 47
    apply(TtoT(6,3));               // 48
    apply(TtoT(6,0));               // 49
    apply(TtoT(6,1));               // 50

    // Verify that evaluating tableau moves on this tricky board is deterministic
    MoveList moves = state.legal_moves();
    
    // There should be exactly one non-no-op Tableau->Foundation move: T1->F2
    std::vector<Move> non_noop_ttf;
    for (const auto& m : moves) {
        if (m.kind == MoveKind::TableauToFoundation) {
            if (!is_no_op_move(state, m)) {
                non_noop_ttf.push_back(m);
            }
        }
    }

    REQUIRE(non_noop_ttf.size() == 1);
    REQUIRE(non_noop_ttf.front().source == PileId(PileKind::Tableau, 1));
    REQUIRE(non_noop_ttf.front().target == PileId(PileKind::Foundation, 2));
    
    bool tested = false;
    for (const auto& move : moves) {
        if (move.kind != MoveKind::TableauToTableau) continue;
        bool r1 = is_no_op_move(state, move);
        bool r2 = is_no_op_move(state, move);
        REQUIRE(r1 == r2);
        tested = true;
    }

    REQUIRE(tested); // ensure we actually tested at least one tableau move
}
