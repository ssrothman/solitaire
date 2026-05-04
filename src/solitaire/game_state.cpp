#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include <algorithm>
#include <sstream>

namespace solitaire {

// ============================================================================
// Constructors
// ============================================================================

GameState::GameState() 
        : _tableau_face_down_cards{},
            _tableau_face_down{}, 
            _foundation{},
            _stock{}, 
            _waste{}, 
            _config(GameConfig()),
            _turn_count(0),
            _stock_cycle_pos(0) {}

GameState::GameState(const GameConfig& cfg) 
        : _tableau_face_down_cards{},
            _tableau_face_down{}, 
            _foundation{},
            _stock{}, 
            _waste{}, 
            _config(cfg),
            _turn_count(0),
            _stock_cycle_pos(0) {}

GameState GameState::from_deck(const std::vector<Card>& deck,
                                const GameConfig& cfg) {
    GameState state(cfg);

    // Deal the cards
    state._deal_from_deck(deck);
    return state;
}

// ============================================================================
// State Queries
// ============================================================================

int GameState::tableau_face_down_count(int pile) const {
    return static_cast<int>(_tableau_face_down_cards[pile].size());
}

Card GameState::tableau_top(int pile) const {
    if (_tableau_face_up[pile].empty()) {
        return Card();  // Empty card
    }
    return _tableau_face_up[pile].front();
}

int GameState::tableau_face_up_count(int pile) const {
    return _tableau_face_up[pile].size();
}

Card GameState::foundation_top(int suit_index) const {
    return _foundation[suit_index];
}

int GameState::foundation_size(int suit_index) const {
    Card top = _foundation[suit_index];
    // Empty foundation uses rank 0 (Card()) and counts as size 0.
    return static_cast<int>(top.rank());
}

int GameState::stock_size() const {
    return _stock.size();
}

int GameState::waste_size() const {
    return _waste.size();
}

Card GameState::waste_top() const {
    if (_waste.empty()) {
        return Card();  // Empty card
    }
    return _waste.front();  // Waste pile front is top
}

bool GameState::has_waste() const {
    return !_waste.empty();
}

// ============================================================================
// Move Generation
// ============================================================================

MoveList GameState::legal_moves() const {
    MoveList moves;

    // Stock moves have highest priority
    _generate_stock_moves(moves);

    // Tableau to foundation
    _generate_tableau_to_foundation_moves(moves);

    // Waste to foundation (if any waste)
    if (has_waste()) {
        Card top = _waste[0];
        if (can_move_to_foundation(top)) {
            Move m;
            m.source = PileId(PileKind::Waste, 0);
            m.target = PileId(PileKind::Foundation, static_cast<int>(top.suit()));
            m.kind = MoveKind::WasteToFoundation;
            m.card_count = 1;
            moves.push_back(m);
        }
    }

    // Tableau to tableau
    _generate_tableau_to_tableau_moves(moves);

    // Waste to tableau
    _generate_waste_moves(moves);

    return moves;
}

bool GameState::is_legal(const Move& move) const {
    if (!move.is_valid()) {
        return false;
    }

    const int src = move.source.index();
    const int tgt = move.target.index();

    switch (move.kind) {
        case MoveKind::TableauToTableau: {
            const auto& src_pile = _tableau_face_up[src];
            if (move.card_count > static_cast<int>(src_pile.size())) {
                return false;
            }

            // legal_moves() treats card_count as a 1-based index into source sequence.
            Card moved_bottom = src_pile[move.card_count - 1];
            return can_move_to_tableau(moved_bottom, tgt);
        }

        case MoveKind::TableauToFoundation: {
            const auto& src_pile = _tableau_face_up[src];
            if (src_pile.empty()) {
                return false;
            }

            Card card = src_pile.front();
            return can_move_to_foundation(card) &&
                   tgt == static_cast<int>(card.suit());
        }

        case MoveKind::WasteToTableau: {
            if (_waste.empty()) {
                return false;
            }

            return can_move_to_tableau(_waste.front(), tgt);
        }

        case MoveKind::WasteToFoundation: {
            if (_waste.empty()) {
                return false;
            }

            Card top = _waste.front();
            return can_move_to_foundation(top) &&
                   tgt == static_cast<int>(top.suit());
        }

        case MoveKind::StockDraw:
            return move.card_count == _config.cards_per_draw && !_stock.empty();

        case MoveKind::StockRecycle:
            return _stock.empty() &&
                   !_waste.empty() &&
                   _config.unlimited_recycle &&
                   move.card_count == static_cast<int>(_waste.size()) &&
                   move.card_count > 0;
    }

    return false;
}

bool GameState::can_move_to_foundation(const Card& card) const {
    int suit_idx = static_cast<int>(card.suit());
    Card top = _foundation[suit_idx];
    const int top_rank = static_cast<int>(top.rank());
    const int card_rank = static_cast<int>(card.rank());

    if (top_rank == 0) {
        // Empty foundation only accepts an Ace of matching suit.
        return card_rank == static_cast<int>(Rank::Ace);
    }

    // Next rank must match and be one higher
    if (top_rank + 1 == card_rank) {
        return true;
    }

    return false;
}

bool GameState::can_move_to_tableau(const Card& card, int tableau_idx) const {
    const auto& pile = _tableau_face_up[tableau_idx];

    if (pile.empty()) {
        // Only Kings can go to empty pile
        return card.rank() == Rank::King;
    }

    // Card must be one rank lower and opposite color
    return card.can_follow_in_tableau(pile.front());
}

// ============================================================================
// Move Application
// ============================================================================

GameState GameState::apply_move(const Move& move) const {
    GameState next(*this);

    switch (move.kind) {
        case MoveKind::TableauToTableau:
            next._move_tableau_to_tableau(move.source.index(), 
                                          move.target.index(),
                                          move.card_count);
            break;
        case MoveKind::TableauToFoundation:
            next._move_tableau_to_foundation(move.source.index());
            break;
        case MoveKind::WasteToTableau:
            next._move_waste_to_tableau(move.target.index());
            break;
        case MoveKind::WasteToFoundation:
            next._move_waste_to_foundation();
            break;
        case MoveKind::StockDraw:
            next._draw_from_stock();
            break;
        case MoveKind::StockRecycle:
            next._recycle_stock();
            break;
    }

    return next;
}

// ============================================================================
// Game Over Detection
// ============================================================================

bool GameState::is_won() const {
    // All 52 cards in foundation
    for (int i = 0; i < NUM_FOUNDATIONS; ++i) {
        if (_foundation[i].rank() != Rank::King) {
            return false;
        }
    }
    return true;
}

bool GameState::is_lost() const {
    // No legal moves and stock exhausted
    if (!_stock.empty()) {
        return false;  // Can still draw
    }

    MoveList moves = legal_moves();
    return moves.empty();
}

// ============================================================================
// State Comparison and Hashing
// ============================================================================

bool GameState::operator==(const GameState& other) const {
    // Compare tableau
    for (int i = 0; i < NUM_TABLEAU_PILES; ++i) {
        if (_tableau_face_up[i] != other._tableau_face_up[i]) {
            return false;
        }
        if (_tableau_face_down_cards[i] != other._tableau_face_down_cards[i]) {
            return false;
        }
        if (_tableau_face_down[i] != other._tableau_face_down[i]) {
            return false;
        }
    }

    // Compare foundation
    for (int i = 0; i < NUM_FOUNDATIONS; ++i) {
        if (_foundation[i] != other._foundation[i]) {
            return false;
        }
    }

    // Compare stock and waste
    if (_stock != other._stock) {
        return false;
    }
    if (_waste != other._waste) {
        return false;
    }

    // Compare metadata
    if (_stock_cycle_pos != other._stock_cycle_pos) {
        return false;
    }

    return true;
}

bool GameState::same_position(const GameState& other) const {
    // Compare tableau
    for (int i = 0; i < NUM_TABLEAU_PILES; ++i) {
        if (_tableau_face_up[i] != other._tableau_face_up[i]) {
            return false;
        }
        if (_tableau_face_down_cards[i] != other._tableau_face_down_cards[i]) {
            return false;
        }
        if (_tableau_face_down[i] != other._tableau_face_down[i]) {
            return false;
        }
    }

    // Compare foundation
    for (int i = 0; i < NUM_FOUNDATIONS; ++i) {
        if (_foundation[i] != other._foundation[i]) {
            return false;
        }
    }

    // Compare stock and waste
    if (_stock != other._stock) {
        return false;
    }
    if (_waste != other._waste) {
        return false;
    }

    // Compare configuration because it changes move semantics.
    if (_config.cards_per_draw != other._config.cards_per_draw ||
        _config.unlimited_recycle != other._config.unlimited_recycle) {
        return false;
    }

    return true;
}

std::size_t GameState::hash() const {
    std::size_t h = 0;

    // Hash tableau (face-up and face-down)
    for (int i = 0; i < NUM_TABLEAU_PILES; ++i) {
        for (const auto& card : _tableau_face_up[i]) {
            h ^= std::hash<Card>()(card) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& card : _tableau_face_down_cards[i]) {
            h ^= std::hash<Card>()(card) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= _tableau_face_down[i] + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    // Hash foundation
    for (int i = 0; i < NUM_FOUNDATIONS; ++i) {
        h ^= std::hash<Card>()(_foundation[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    // Hash stock and waste
    for (const auto& card : _stock) {
        h ^= std::hash<Card>()(card) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    for (const auto& card : _waste) {
        h ^= std::hash<Card>()(card) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    // Hash cycle position
    h ^= _stock_cycle_pos + 0x9e3779b9 + (h << 6) + (h >> 2);

    return h;
}

std::string GameState::to_string() const {
    std::stringstream ss;
    ss << "=== GameState ===\n";
    ss << "Tableau:\n";
    for (int i = 0; i < NUM_TABLEAU_PILES; ++i) {
        ss << "  T" << i << ": ";
        for (const auto& card : _tableau_face_up[i]) {
            ss << card.to_string() << " ";
        }
        if (!_tableau_face_down_cards[i].empty()) {
            ss << "[" << _tableau_face_down_cards[i].size() << " hidden]";
        }
        ss << "\n";
    }
    ss << "Foundation: ";
    for (int i = 0; i < NUM_FOUNDATIONS; ++i) {
        if (foundation_size(i) == 0) {
            ss << "-- ";
        } else {
            ss << _foundation[i].to_string() << " ";
        }
    }
    ss << "\n";
    ss << "Stock: " << _stock.size() << " cards\n";
    ss << "Waste: ";
    if (has_waste()) {
        ss << waste_top().to_string();
    } else {
        ss << "empty";
    }
    ss << "\n";
    return ss.str();
}

// ============================================================================
// Internal Helpers
// ============================================================================

void GameState::_deal_from_deck(const std::vector<Card>& deck) {
    int card_idx = 0;

    // Deal tableau: 1, 2, 3, 4, 5, 6, 7 cards per pile
    for (int pile = 0; pile < NUM_TABLEAU_PILES; ++pile) {
        for (int i = 0; i < pile; ++i) {
            _tableau_face_down_cards[pile].push_back(deck[card_idx]);
            _tableau_face_down[pile]++;
            card_idx++;
        }
        // Top card is face-up
        if (card_idx < static_cast<int>(deck.size())) {
            _tableau_face_up[pile].push_back(deck[card_idx++]);
        }
    }

    // Remaining cards go to stock
    while (card_idx < static_cast<int>(deck.size())) {
        _stock.push_back(deck[card_idx++]);
    }

    // Foundations remain empty (Card()) until an Ace is moved in.
}

void GameState::_move_tableau_to_tableau(int src_pile, int tgt_pile,
                                          int card_count) {
    auto& src = _tableau_face_up[src_pile];
    auto& tgt = _tableau_face_up[tgt_pile];

    // Move cards
    for (int i = 0; i < card_count; ++i) {
        tgt.push_front(src.front());
        src.pop_front();
    }

    // Reveal if source is now empty
    if (src.empty()) {
        _reveal_tableau_card(src_pile);
    }
}

void GameState::_move_tableau_to_foundation(int src_pile) {
    auto& src = _tableau_face_up[src_pile];
    if (src.empty()) {
        return;
    }

    Card card = src.front();
    src.pop_front();

    int suit_idx = static_cast<int>(card.suit());
    _foundation[suit_idx] = card;

    if (src.empty()) {
        _reveal_tableau_card(src_pile);
    }
}

void GameState::_move_waste_to_tableau(int tgt_pile) {
    if (_waste.empty()) {
        return;
    }

    Card card = _waste.front();
    _waste.pop_front();

    auto& tgt = _tableau_face_up[tgt_pile];
    tgt.push_front(card);
}

void GameState::_move_waste_to_foundation() {
    if (_waste.empty()) {
        return;
    }

    Card card = _waste.front();
    _waste.pop_front();

    int suit_idx = static_cast<int>(card.suit());
    _foundation[suit_idx] = card;
}

void GameState::_draw_from_stock() {
    if (_stock.empty()) {
        return;
    }

    // Draw cards_per_draw cards from stock to waste
    int draw_count = _config.cards_per_draw;
    for (int i = 0; i < draw_count && !_stock.empty(); ++i) {
        Card card = _stock.back();
        _stock.pop_back();
        _waste.push_front(card);
    }
}

void GameState::_recycle_stock() {
    if (!_config.unlimited_recycle) {
        return;
    }

    // Move waste back to stock
    while (!_waste.empty()) {
        _stock.push_back(_waste.front());
        _waste.pop_front();
    }

    _turn_count++;
    _stock_cycle_pos++;
}

void GameState::_reveal_tableau_card(int pile) {
    if (!_tableau_face_down_cards[pile].empty()) {
        Card revealed = _tableau_face_down_cards[pile].back();
        _tableau_face_down_cards[pile].pop_back();
        _tableau_face_up[pile].push_front(revealed);
    }

    if (_tableau_face_down[pile] > 0) {
        _tableau_face_down[pile]--;
    }
}

void GameState::_generate_tableau_to_tableau_moves(MoveList& moves) const {
    for (int src = 0; src < NUM_TABLEAU_PILES; ++src) {
        if (_tableau_face_up[src].empty()) {
            continue;
        }

        // Try each card in the sequence
        int seq_len = _tableau_face_up[src].size();
        for (int i = 0; i < seq_len; ++i) {
            Card card = _tableau_face_up[src][i];
            int move_count = i + 1;

            for (int tgt = 0; tgt < NUM_TABLEAU_PILES; ++tgt) {
                if (src == tgt) {
                    continue;
                }

                if (can_move_to_tableau(card, tgt)) {
                    Move m;
                    m.source = PileId(PileKind::Tableau, src);
                    m.target = PileId(PileKind::Tableau, tgt);
                    m.kind = MoveKind::TableauToTableau;
                    m.card_count = move_count;
                    moves.push_back(m);
                }
            }
        }
    }
}

void GameState::_generate_tableau_to_foundation_moves(MoveList& moves) const {
    for (int src = 0; src < NUM_TABLEAU_PILES; ++src) {
        if (_tableau_face_up[src].empty()) {
            continue;
        }

        Card card = _tableau_face_up[src].front();
        if (can_move_to_foundation(card)) {
            Move m;
            m.source = PileId(PileKind::Tableau, src);
            m.target = PileId(PileKind::Foundation, static_cast<int>(card.suit()));
            m.kind = MoveKind::TableauToFoundation;
            m.card_count = 1;
            moves.push_back(m);
        }
    }
}

void GameState::_generate_waste_moves(MoveList& moves) const {
    if (_waste.empty()) {
        return;
    }

    Card top = _waste[0];

    // Waste to tableau
    for (int tgt = 0; tgt < NUM_TABLEAU_PILES; ++tgt) {
        if (can_move_to_tableau(top, tgt)) {
            Move m;
            m.source = PileId(PileKind::Waste, 0);
            m.target = PileId(PileKind::Tableau, tgt);
            m.kind = MoveKind::WasteToTableau;
            m.card_count = 1;
            moves.push_back(m);
        }
    }
}

void GameState::_generate_stock_moves(MoveList& moves) const {
    if (!_stock.empty()) {
        // Can draw from stock
        Move m;
        m.source = PileId(PileKind::Stock, 0);
        m.target = PileId(PileKind::Waste, 0);
        m.kind = MoveKind::StockDraw;
        m.card_count = _config.cards_per_draw;
        moves.push_back(m);
    } else if (!_waste.empty() && _config.unlimited_recycle) {
        // Can recycle waste to stock
        Move m;
        m.source = PileId(PileKind::Waste, 0);
        m.target = PileId(PileKind::Stock, 0);
        m.kind = MoveKind::StockRecycle;
        m.card_count = static_cast<int>(_waste.size());
        moves.push_back(m);
    }
}

}  // namespace solitaire
