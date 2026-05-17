#ifndef SOLITAIRE_TYPES_H
#define SOLITAIRE_TYPES_H

// Human-reviewed: do not edit further with AI assistance unless explicitly confirmed.

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "constants.h"

namespace solitaire {

// ============================================================================
// Basic Card Types
// ============================================================================

enum class Suit : std::uint8_t {
    Hearts = 0,
    Diamonds = 1,
    Clubs = 2,
    Spades = 3,
};

enum class Rank : std::uint8_t {
    Invalid = 0,  // Used for empty cards
    Ace = 1,
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    Six = 6,
    Seven = 7,
    Eight = 8,
    Nine = 9,
    Ten = 10,
    Jack = 11,
    Queen = 12,
    King = 13,
};

enum class Color : std::uint8_t {
    Red = 0,
    Black = 1,
};

// Get color from suit (optimized: Hearts/Diamonds are 0-1, Clubs/Spades are 2-3)
constexpr Color suit_to_color(Suit s) {
    return (static_cast<int>(s) < 2) ? Color::Red : Color::Black;
}

// Card: Single uint8_t internally, accessed via member functions
// Layout: bits 4-5 = suit (0-3), bits 0-3 = rank (1-13)
class Card {
    std::uint8_t _data;

public:
    constexpr Card() : _data(0) {}  // Empty card
    constexpr explicit Card(std::uint8_t d) : _data(d) {}
    constexpr Card(Suit s, Rank r) 
        : _data((static_cast<std::uint8_t>(s) << 4) | static_cast<std::uint8_t>(r)) {}

    constexpr Suit suit() const {
        return static_cast<Suit>(_data >> 4);
    }

    constexpr Rank rank() const {
        return static_cast<Rank>(_data & 0b00001111);
    }

    constexpr Color color() const {
        return suit_to_color(suit());
    }

    // Returns true if this card can be placed on another in tableau
    // (this must be one rank lower and opposite color)
    constexpr bool can_follow_in_tableau(const Card& other) const {
        return static_cast<int>(rank()) == static_cast<int>(other.rank()) - 1 &&
               color() != other.color();
    }

    constexpr bool operator==(const Card& other) const {
        return _data == other._data;
    }

    constexpr bool operator!=(const Card& other) const {
        return _data != other._data;
    }

    // For hash functions and direct access
    constexpr std::uint8_t raw_data() const { return _data; }

    std::string to_string() const;
};
static_assert(sizeof(Card) == 1);  // Ensure Card is exactly 1 byte for compact storage

// ============================================================================
// Move Types
// ============================================================================

enum class MoveKind : std::uint8_t {
    TableauToTableau = 0,
    TableauToFoundation = 1,
    WasteToTableau = 2,
    WasteToFoundation = 3,
    StockDraw = 4,
    StockRecycle = 5,
};

enum class PileKind : std::uint8_t {
    Tableau = 0,
    Foundation = 1,
    Stock = 2,
    Waste = 3,
};

// PileId: Single uint8_t internally, accessed via member functions
// Layout: bits 3-4 = kind (0-3), bits 0-2 = index (0-7)
class PileId {
    std::uint8_t _data;

public:
    constexpr PileId() : _data(0) {}  // Tableau 0
    constexpr explicit PileId(std::uint8_t d) : _data(d) {}
    constexpr PileId(PileKind k, int i) 
        : _data((static_cast<std::uint8_t>(k) << 3) | (static_cast<std::uint8_t>(i) & 0x07)) {}

    constexpr PileKind kind() const {
        return static_cast<PileKind>(_data >> 3);
    }

    constexpr int index() const {
        return _data & 0x07;
    }

    constexpr bool operator==(const PileId& other) const {
        return _data == other._data;
    }

    constexpr bool operator!=(const PileId& other) const {
        return _data != other._data;
    }

    // For hash functions and direct access
    constexpr std::uint8_t raw_data() const { return _data; }

    std::string to_string() const;
};
static_assert(sizeof(PileId) == 1);  // Ensure PileId is exactly 1 byte for compact storage

struct Move {
    PileId source;
    PileId target;
    MoveKind kind;
    std::uint8_t card_count;  // For tableau sequences, how many cards to move

    Move() : source(), target(), kind(MoveKind::StockDraw), card_count(0) {}
    Move(PileId src, PileId tgt, MoveKind k, std::uint8_t cnt)
        : source(src), target(tgt), kind(k), card_count(cnt) {}

    // Checks move shape only (pile kinds/indices/card_count), not game-state legality.
    bool is_valid() const;

    // Equality operators for testing and hashing
    bool operator==(const Move& other) const {
        return source == other.source &&
               target == other.target &&
               kind == other.kind &&
               card_count == other.card_count;
    }

    std::string to_string() const;

    size_t hash() const {
        size_t h = source.raw_data();
        h = h << 8 | target.raw_data();
        h = h << 3 | static_cast<size_t>(kind);
        h = h << 8 | static_cast<size_t>(card_count);
        return h;
    } 
};
static_assert(sizeof(Move) == 4);  // Ensure Move is exactly 4 bytes for compact storage

using MoveList = std::vector<Move>;

// ============================================================================
// Configuration
// ============================================================================

// packed into a single uint8_t
// format: bits 0-2 = cards_per_draw (0-7), bit 3 = unlimited_recycle (0 or 1), bits 4-7 unused
struct GameConfig {
    uint8_t _data = (DEFAULT_UNLIMITED_RECYCLE << 3) | (DEFAULT_CARDS_PER_DRAW); // Internal packed representation

    uint8_t cards_per_draw() const { return _data & 0b00000111; }
    bool unlimited_recycle() const { return (_data & 0b00001000) != 0; }

    GameConfig() = default;
    explicit GameConfig(int cpd, bool ur = DEFAULT_UNLIMITED_RECYCLE)
        : _data((ur << 3) | (cpd & 0b00000111)) {}
};
static_assert(sizeof(GameConfig) == 1);  // Ensure GameConfig is exactly 1 byte for compact storage

// ============================================================================
// Solver Result Types
// ============================================================================

struct SearchStats {
    int nodes_explored = 0;
    int max_depth = 0;
    double time_seconds = 0.0;
    int moves_pruned = 0;  // Optional: tracks pruned moves for metrics
};

enum class SolverStatus : std::uint8_t {
    Success = 0,
    ReachedMaxDepth = 1,
    ReachedMaxNodes = 2,
    InvalidState = 3
};

std::string to_string(SolverStatus status);

struct SolverResult {
    bool solvable = false;
    SolverStatus status = SolverStatus::InvalidState;  // Initialize to invalid - solver should always overwrite this
    MoveList solution;  // Empty if unsolvable
    SearchStats stats;

    std::string to_string() const;
};

struct PolicyResult {
    bool won = false;
    MoveList moves;  // Sequence of moves executed
    int turns = 0;   // How many turns (stock cycles)
    SearchStats stats;
};

struct SolverConfig {
    int max_depth = DEFAULT_SOLVER_MAX_DEPTH;                            // Max depth before giving up
    int max_nodes = DEFAULT_SOLVER_MAX_NODES;                          // Max nodes to explore (100M)
    int seed = DEFAULT_SOLVER_SEED;                                    // RNG seed for randomized solvers
    bool enable_productive_move_pruning = true;  // Prune moves that don't progress towards solution
    bool productive_move_pruning_TtoT_DFS = false;  // Whether to run DFS analysis for T-to-T moves in pruning
};

}  // namespace solitaire

// ============================================================================
// Standard library hashing for solver visited-state tracking
// ============================================================================

namespace std {

template <>
struct hash<solitaire::Card> {
    std::size_t operator()(const solitaire::Card& c) const {
        return static_cast<std::size_t>(c.raw_data());
    }
};

template <>
struct hash<solitaire::PileId> {
    std::size_t operator()(const solitaire::PileId& p) const {
        return static_cast<std::size_t>(p.raw_data());
    }
};

template <>
struct hash<solitaire::Move> {
    std::size_t operator()(const solitaire::Move& m) const {
        return m.hash();
    }
};

}  // namespace std

#endif  // SOLITAIRE_TYPES_H
