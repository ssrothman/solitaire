#ifndef SOLITAIRE_SHUFFLE_H
#define SOLITAIRE_SHUFFLE_H

#include "types.h"
#include <vector>
#include <random>

namespace solitaire {

class GameState;  // Forward declaration

// ============================================================================
// Shuffle and Deal
// ============================================================================

class Deck {
public:
    Deck();

    // Create a shuffled deck with given seed
    static Deck shuffled(unsigned int seed);

    // Get next card from deck
    Card draw();

    // Check if deck has cards
    bool has_cards() const;

private:
    std::vector<Card> _cards;
    size_t _position;

    Deck(const std::vector<Card>& cards) : _cards(cards), _position(0) {}
};

// Shuffle a standard 52-card deck with given seed
// Returns vector of Cards in shuffled order
std::vector<Card> shuffle_deck(unsigned int seed);

// Create a game state from a shuffled deck
GameState deal_game(unsigned int seed, const GameConfig& cfg = GameConfig());

}  // namespace solitaire

#endif  // SOLITAIRE_SHUFFLE_H
