#ifndef SOLITAIRE_GAME_STATE_H
#define SOLITAIRE_GAME_STATE_H

#include "types.h"
#include "constants.h"
#include <vector>
#include <deque>
#include <array>
#include <cstddef>

namespace solitaire {

// ============================================================================
// GameState - Immutable game state representation
// ============================================================================

class GameState {
public:
    // ========================================================================
    // Constructors and Factory Methods
    // ========================================================================

    GameState();
    explicit GameState(const GameConfig& cfg);

    // Create a game state from a deck of Cards in shuffled order
    static GameState from_deck(const std::vector<Card>& deck, const GameConfig& cfg);

    // Copy constructor and assignment
    GameState(const GameState& other) = default;
    GameState& operator=(const GameState& other) = default;

    // Move semantics
    GameState(GameState&& other) noexcept = default;
    GameState& operator=(GameState&& other) noexcept = default;

    // ========================================================================
    // State Queries
    // ========================================================================

    // Tableau queries  
    int tableau_face_down_count(int pile) const;
    Card tableau_top(int pile) const;  // Get top face-up card, or empty Card if none
    int tableau_face_up_count(int pile) const;  // How many face-up cards in pile

    // Foundation queries
    Card foundation_top(int suit_index) const;
    int foundation_size(int suit_index) const;

    // Stock and waste
    int stock_size() const;
    int waste_size() const;
    Card waste_top() const;
    bool has_waste() const;

    // Configuration
    const GameConfig& config() const { return _config; }

    // Metadata
    int turn_count() const { return _turn_count; }
    int stock_cycle_position() const { return _stock_cycle_pos; }

    // ========================================================================
    // Move Generation and Validation
    // ========================================================================

    // Get all legal moves in current state
    MoveList legal_moves() const;

    // Check if a specific move is legal
    bool is_legal(const Move& move) const;

    // Check specific move types
    bool can_move_to_foundation(const Card& card) const;
    bool can_move_to_tableau(const Card& card, int tableau_idx) const;

    // ========================================================================
    // Move Application
    // ========================================================================

    // Apply a move and return the new state
    // Asserts that the move is legal
    GameState apply_move(const Move& move) const;

    // ========================================================================
    // Game Over Detection
    // ========================================================================

    bool is_won() const;
    bool is_lost() const;

    // ========================================================================
    // State Comparison and Hashing
    // ========================================================================

    bool operator==(const GameState& other) const;
    bool operator!=(const GameState& other) const { return !(*this == other); }

    // Compare only gameplay-relevant state, ignoring turn/cycle metadata.
    bool same_position(const GameState& other) const;

    // Hash for visited-state tracking in solvers
    // Includes stock cycle position to avoid false merges
    std::size_t hash() const;

    std::string to_string() const;

private:
    // ========================================================================
    // Internal State
    // ========================================================================

    // Tableau: 7 piles, each with face-up cards (front) and face-down count
    std::array<std::deque<Card>, NUM_TABLEAU_PILES> _tableau_face_up;
    std::array<int, NUM_TABLEAU_PILES> _tableau_face_down;

    // Foundation: 4 piles (one per suit), top card or empty
    std::array<Card, NUM_FOUNDATIONS> _foundation;

    // Stock (draw pile)
    std::deque<Card> _stock;

    // Waste pile
    std::deque<Card> _waste;

    // Configuration
    GameConfig _config;

    // Metadata for solver state hashing
    int _turn_count = 0;        // Increments each stock cycle
    int _stock_cycle_pos = 0;   // Current offset in stock recycle sequence

    // ========================================================================
    // Internal Helper Methods
    // ========================================================================

    // Initialize from deck
    void _deal_from_deck(const std::vector<Card>& deck);

    // Move application helpers
    void _move_tableau_to_tableau(int src_pile, int tgt_pile, int card_count);
    void _move_tableau_to_foundation(int src_pile);
    void _move_waste_to_tableau(int tgt_pile);
    void _move_waste_to_foundation();
    void _draw_from_stock();
    void _recycle_stock();

    // Flip newly exposed tableau cards
    void _reveal_tableau_card(int pile);

    // Move generation helpers
    void _generate_tableau_to_tableau_moves(MoveList& moves) const;
    void _generate_tableau_to_foundation_moves(MoveList& moves) const;
    void _generate_waste_moves(MoveList& moves) const;
    void _generate_stock_moves(MoveList& moves) const;
};

// ============================================================================
// Helper Functions
// ============================================================================

}  // namespace solitaire

#endif  // SOLITAIRE_GAME_STATE_H
