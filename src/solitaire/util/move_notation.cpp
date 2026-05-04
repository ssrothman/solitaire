// Human-reviewed: do not edit further with AI assistance unless explicitly confirmed.
#include "solitaire/util/move_notation.h"
#include <charconv>
#include <sstream>
#include <string_view>

namespace solitaire::util {

namespace {

std::optional<int> parse_int(std::string_view text) {
    int value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

std::optional<PileId> parse_pile(std::string_view text) {
    if (text == "S") {
        return PileId(PileKind::Stock, 0);
    }
    if (text == "W") {
        return PileId(PileKind::Waste, 0);
    }
    if (text == "F") {
        // Bare "F" is accepted as a placeholder; will be inferred later
        return PileId(PileKind::Foundation, 0);
    }
    if (text.size() < 2) {
        return std::nullopt;
    }

    PileKind kind;
    if (text[0] == 'F') {
        kind = PileKind::Foundation;
    } else if (text[0] == 'T') {
        kind = PileKind::Tableau;
    } else {
        return std::nullopt;
    }

    auto index = parse_int(text.substr(1));
    if (!index || *index < 0 || *index > 7) {
        return std::nullopt;
    }

    return PileId(kind, *index);
}

}  // namespace

std::string move_to_notation(const Move& move) {
    std::ostringstream oss;
    
    std::string src_not = move.source.to_string();
    std::string tgt_not = move.target.to_string();
    
    switch (move.kind) {
        case solitaire::MoveKind::StockDraw:
            oss << "Draw";
            if (move.card_count > 1) {
                oss << "(" << move.card_count << ")";
            }
            break;
        case solitaire::MoveKind::StockRecycle:
            oss << "Recycle";
            break;
        case solitaire::MoveKind::TableauToTableau:
            oss << src_not << "→" << tgt_not;
            if (move.card_count > 1) {
                oss << "(" << move.card_count << ")";
            }
            break;
        case solitaire::MoveKind::TableauToFoundation:
            oss << src_not << "→" << tgt_not;
            break;
        case solitaire::MoveKind::WasteToTableau:
            oss << "W→" << tgt_not;
            break;
        case solitaire::MoveKind::WasteToFoundation:
            oss << "W→" << tgt_not;
            break;
    }
    
    return oss.str();
}

    std::optional<Move> move_from_notation(std::string_view notation) {
        if (notation == "Draw") {
            return Move(PileId(PileKind::Stock, 0), PileId(PileKind::Waste, 0), MoveKind::StockDraw, 1);
        }
        if (notation == "Recycle") {
            return Move(PileId(PileKind::Waste, 0), PileId(PileKind::Stock, 0), MoveKind::StockRecycle, 0);
        }

        if (notation.starts_with("Draw(") && notation.ends_with(')')) {
            auto count = parse_int(notation.substr(5, notation.size() - 6));
            if (!count || *count <= 0) {
                return std::nullopt;
            }
            return Move(PileId(PileKind::Stock, 0), PileId(PileKind::Waste, 0), MoveKind::StockDraw, *count);
        }

        // Try to find arrow: first try "->" (2 bytes) then "→" (UTF-8, 3 bytes)
        size_t arrow_pos = notation.find("->");
        size_t arrow_len = 2;
        if (arrow_pos == std::string_view::npos) {
            // Try UTF-8 arrow: the literal "→" which is 3 bytes in UTF-8
            constexpr std::string_view utf8_arrow = "→";
            arrow_pos = notation.find(utf8_arrow);
            arrow_len = utf8_arrow.size();  // 3 bytes
            if (arrow_pos == std::string_view::npos) {
                return std::nullopt;
            }
        }

        std::string_view src_text = notation.substr(0, arrow_pos);
        std::string_view rest = notation.substr(arrow_pos + arrow_len);

        int card_count = 1;
        auto count_pos = rest.find('(');
        if (count_pos != std::string_view::npos) {
            if (!rest.ends_with(')')) {
                return std::nullopt;
            }
            auto count = parse_int(rest.substr(count_pos + 1, rest.size() - count_pos - 2));
            if (!count || *count <= 0) {
                return std::nullopt;
            }
            card_count = *count;
            rest = rest.substr(0, count_pos);
        }

        auto src = parse_pile(src_text);
        auto tgt = parse_pile(rest);
        if (!src || !tgt) {
            return std::nullopt;
        }

        MoveKind kind;
        if (src->kind() == PileKind::Tableau && tgt->kind() == PileKind::Tableau) {
            kind = MoveKind::TableauToTableau;
        } else if (src->kind() == PileKind::Tableau && tgt->kind() == PileKind::Foundation) {
            kind = MoveKind::TableauToFoundation;
            if (card_count != 1) {
                return std::nullopt;
            }
        } else if (src->kind() == PileKind::Waste && tgt->kind() == PileKind::Tableau) {
            kind = MoveKind::WasteToTableau;
            if (card_count != 1) {
                return std::nullopt;
            }
        } else if (src->kind() == PileKind::Waste && tgt->kind() == PileKind::Foundation) {
            kind = MoveKind::WasteToFoundation;
            if (card_count != 1) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }

        return Move(*src, *tgt, kind, card_count);
    }

}  // namespace solitaire::util
