#include "solitaire/shuffle.h"
#include "solitaire/constants.h"
#include "solitaire/game_state.h"
#include <algorithm>
#include <random>

namespace solitaire {

// ============================================================================
// Deck Implementation
// ============================================================================

Deck::Deck() {
    // Initialize with standard 52 cards
    for (int s = 0; s < NUM_SUITS; ++s) {
        for (int r = 1; r <= NUM_RANKS; ++r) {
            _cards.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
    _position = 0;  // Initialize position counter
}

Deck Deck::shuffled(unsigned int seed) {
    Deck deck;
    std::mt19937 rng(seed);
    std::shuffle(deck._cards.begin(), deck._cards.end(), rng);
    return deck;
}

Card Deck::draw() {
    if (_position >= _cards.size()) {
        return Card();  // Empty card
    }
    return _cards[_position++];
}

bool Deck::has_cards() const {
    return _position < _cards.size();
}

// ============================================================================
// Shuffle Functions
// ============================================================================

std::vector<Card> shuffle_deck(unsigned int seed) {
    Deck deck = Deck::shuffled(seed);
    std::vector<Card> result;

    while (deck.has_cards()) {
        result.push_back(deck.draw());
    }

    return result;
}

GameState deal_game(unsigned int seed, const GameConfig& cfg) {
    auto deck = shuffle_deck(seed);
    return GameState::from_deck(deck, cfg);
}

}  // namespace solitaire
