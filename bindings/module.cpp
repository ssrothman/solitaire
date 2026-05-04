#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/solvers/complete_dfs.h"
#include "solitaire/solvers/greedy_policy.h"
#include "solitaire/solvers/random_policy.h"
#include "solitaire/solvers/solver.h"
#include "solitaire/util/move_notation.h"

namespace py = pybind11;

namespace {

class PyIPolicy : public solitaire::solvers::IPolicy {
public:
    using solitaire::solvers::IPolicy::IPolicy;

    solitaire::Move choose_move(const solitaire::GameState& state,
                                const solitaire::MoveList& legal_moves) override {
        PYBIND11_OVERRIDE_PURE(
            solitaire::Move,
            solitaire::solvers::IPolicy,
            choose_move,
            state,
            legal_moves);
    }

    double evaluate_move(const solitaire::GameState& state,
                         const solitaire::Move& move) override {
        PYBIND11_OVERRIDE(
            double,
            solitaire::solvers::IPolicy,
            evaluate_move,
            state,
            move);
    }
};

template <typename Enum>
void bind_enum(py::module_& m,
               const char* name,
               std::initializer_list<std::pair<const char*, Enum>> values) {
    auto e = py::enum_<Enum>(m, name);
    for (const auto& [label, value] : values) {
        e.value(label, value);
    }
    e.export_values();
}

}  // namespace

PYBIND11_MODULE(_solitaire_binding, m) {
    m.doc() = "Solitaire solver library bindings";

    bind_enum<solitaire::Suit>(m, "Suit", {
        {"Hearts", solitaire::Suit::Hearts},
        {"Diamonds", solitaire::Suit::Diamonds},
        {"Clubs", solitaire::Suit::Clubs},
        {"Spades", solitaire::Suit::Spades},
    });

    bind_enum<solitaire::Rank>(m, "Rank", {
        {"Ace", solitaire::Rank::Ace},
        {"Two", solitaire::Rank::Two},
        {"Three", solitaire::Rank::Three},
        {"Four", solitaire::Rank::Four},
        {"Five", solitaire::Rank::Five},
        {"Six", solitaire::Rank::Six},
        {"Seven", solitaire::Rank::Seven},
        {"Eight", solitaire::Rank::Eight},
        {"Nine", solitaire::Rank::Nine},
        {"Ten", solitaire::Rank::Ten},
        {"Jack", solitaire::Rank::Jack},
        {"Queen", solitaire::Rank::Queen},
        {"King", solitaire::Rank::King},
    });

    bind_enum<solitaire::Color>(m, "Color", {
        {"Red", solitaire::Color::Red},
        {"Black", solitaire::Color::Black},
    });

    bind_enum<solitaire::MoveKind>(m, "MoveKind", {
        {"TableauToTableau", solitaire::MoveKind::TableauToTableau},
        {"TableauToFoundation", solitaire::MoveKind::TableauToFoundation},
        {"WasteToTableau", solitaire::MoveKind::WasteToTableau},
        {"WasteToFoundation", solitaire::MoveKind::WasteToFoundation},
        {"StockDraw", solitaire::MoveKind::StockDraw},
        {"StockRecycle", solitaire::MoveKind::StockRecycle},
    });

    bind_enum<solitaire::PileKind>(m, "PileKind", {
        {"Tableau", solitaire::PileKind::Tableau},
        {"Foundation", solitaire::PileKind::Foundation},
        {"Stock", solitaire::PileKind::Stock},
        {"Waste", solitaire::PileKind::Waste},
    });

    py::class_<solitaire::Card>(m, "Card")
        .def(py::init<>())
        .def(py::init<std::uint8_t>())
        .def(py::init<solitaire::Suit, solitaire::Rank>())
        .def_property_readonly("suit", &solitaire::Card::suit)
        .def_property_readonly("rank", &solitaire::Card::rank)
        .def_property_readonly("color", &solitaire::Card::color)
        .def("can_follow_in_tableau", &solitaire::Card::can_follow_in_tableau)
        .def_property_readonly("raw_data", &solitaire::Card::raw_data)
        .def("to_string", &solitaire::Card::to_string)
        .def("__repr__", [](const solitaire::Card& card) { return card.to_string(); });

    py::class_<solitaire::PileId>(m, "PileId")
        .def(py::init<>())
        .def(py::init<std::uint8_t>())
        .def(py::init<solitaire::PileKind, int>())
        .def_property_readonly("kind", &solitaire::PileId::kind)
        .def_property_readonly("index", &solitaire::PileId::index)
        .def_property_readonly("raw_data", &solitaire::PileId::raw_data)
        .def("to_string", &solitaire::PileId::to_string)
        .def("__repr__", [](const solitaire::PileId& pile) { return pile.to_string(); });

    py::class_<solitaire::Move>(m, "Move")
        .def(py::init<>())
        .def(py::init<solitaire::PileId, solitaire::PileId, solitaire::MoveKind, int>())
        .def_readwrite("source", &solitaire::Move::source)
        .def_readwrite("target", &solitaire::Move::target)
        .def_readwrite("kind", &solitaire::Move::kind)
        .def_readwrite("card_count", &solitaire::Move::card_count)
        .def("is_valid", &solitaire::Move::is_valid)
        .def("to_string", &solitaire::Move::to_string)
        .def("__eq__", [](const solitaire::Move& a, const solitaire::Move& b) {
            return a.source == b.source &&
                   a.target == b.target &&
                   a.kind == b.kind &&
                   a.card_count == b.card_count;
        })
        .def("__ne__", [](const solitaire::Move& a, const solitaire::Move& b) {
            return a.source != b.source ||
                   a.target != b.target ||
                   a.kind != b.kind ||
                   a.card_count != b.card_count;
        })
        .def("__repr__", [](const solitaire::Move& move) { return move.to_string(); });

    py::class_<solitaire::GameConfig>(m, "GameConfig")
        .def(py::init<>())
        .def(py::init<int, bool>(), py::arg("cards_per_draw"), py::arg("unlimited_recycle") = solitaire::DEFAULT_UNLIMITED_RECYCLE)
        .def_readwrite("cards_per_draw", &solitaire::GameConfig::cards_per_draw)
        .def_readwrite("unlimited_recycle", &solitaire::GameConfig::unlimited_recycle);

    py::class_<solitaire::SearchStats>(m, "SearchStats")
        .def(py::init<>())
        .def_readwrite("nodes_explored", &solitaire::SearchStats::nodes_explored)
        .def_readwrite("max_depth", &solitaire::SearchStats::max_depth)
        .def_readwrite("time_seconds", &solitaire::SearchStats::time_seconds)
        .def_readwrite("moves_pruned", &solitaire::SearchStats::moves_pruned);

    py::class_<solitaire::SolverResult>(m, "SolverResult")
        .def(py::init<>())
        .def_readwrite("solvable", &solitaire::SolverResult::solvable)
        .def_readwrite("solution", &solitaire::SolverResult::solution)
        .def_readwrite("stats", &solitaire::SolverResult::stats);

    py::class_<solitaire::PolicyResult>(m, "PolicyResult")
        .def(py::init<>())
        .def_readwrite("won", &solitaire::PolicyResult::won)
        .def_readwrite("moves", &solitaire::PolicyResult::moves)
        .def_readwrite("turns", &solitaire::PolicyResult::turns)
        .def_readwrite("stats", &solitaire::PolicyResult::stats);

    py::class_<solitaire::SolverConfig>(m, "SolverConfig")
        .def(py::init<>())
        .def_readwrite("max_depth", &solitaire::SolverConfig::max_depth)
        .def_readwrite("max_nodes", &solitaire::SolverConfig::max_nodes)
        .def_readwrite("enable_pruning", &solitaire::SolverConfig::enable_pruning)
        .def_readwrite("seed", &solitaire::SolverConfig::seed)
        .def_readwrite("enable_move_equivalence_pruning", &solitaire::SolverConfig::enable_move_equivalence_pruning)
        .def_readwrite("enable_no_op_pruning", &solitaire::SolverConfig::enable_no_op_pruning);

    py::class_<solitaire::GameState>(m, "GameState")
        .def(py::init<>())
        .def(py::init<const solitaire::GameConfig&>())
        .def_static("from_deck", &solitaire::GameState::from_deck,
                    py::arg("deck"), py::arg("cfg"))
        .def("tableau_face_down_count", &solitaire::GameState::tableau_face_down_count)
        .def("tableau_top", &solitaire::GameState::tableau_top)
        .def("tableau_face_up_count", &solitaire::GameState::tableau_face_up_count)
        .def("foundation_top", &solitaire::GameState::foundation_top)
        .def("foundation_size", &solitaire::GameState::foundation_size)
        .def("stock_size", &solitaire::GameState::stock_size)
        .def("waste_size", &solitaire::GameState::waste_size)
        .def("waste_top", &solitaire::GameState::waste_top)
        .def("has_waste", &solitaire::GameState::has_waste)
        .def_property_readonly("config", &solitaire::GameState::config, py::return_value_policy::reference_internal)
        .def_property_readonly("turn_count", &solitaire::GameState::turn_count)
        .def_property_readonly("stock_cycle_position", &solitaire::GameState::stock_cycle_position)
        .def("legal_moves", &solitaire::GameState::legal_moves)
        .def("is_legal", &solitaire::GameState::is_legal)
        .def("can_move_to_foundation", &solitaire::GameState::can_move_to_foundation)
        .def("can_move_to_tableau", &solitaire::GameState::can_move_to_tableau)
        .def("apply_move", &solitaire::GameState::apply_move)
        .def("is_won", &solitaire::GameState::is_won)
        .def("is_lost", &solitaire::GameState::is_lost)
        .def("hash", &solitaire::GameState::hash)
        .def("to_string", &solitaire::GameState::to_string)
        .def("__eq__", [](const solitaire::GameState& a, const solitaire::GameState& b) {
            return a == b;
        })
        .def("__ne__", [](const solitaire::GameState& a, const solitaire::GameState& b) {
            return a != b;
        })
        .def("__repr__", [](const solitaire::GameState& state) { return state.to_string(); });

    py::class_<solitaire::Deck>(m, "Deck")
        .def(py::init<>())
        .def_static("shuffled", &solitaire::Deck::shuffled)
        .def("draw", &solitaire::Deck::draw)
        .def("has_cards", &solitaire::Deck::has_cards);

    m.def("shuffle_deck", &solitaire::shuffle_deck, py::arg("seed"));
    m.def("deal_game", &solitaire::deal_game,
          py::arg("seed"), py::arg("cfg") = solitaire::GameConfig());
    m.def("move_to_notation", &solitaire::util::move_to_notation);
    m.def("move_from_notation", &solitaire::util::move_from_notation);

    py::class_<solitaire::solvers::IPolicy, PyIPolicy, std::shared_ptr<solitaire::solvers::IPolicy>>(m, "IPolicy")
        .def("choose_move", &solitaire::solvers::IPolicy::choose_move)
        .def("evaluate_move", &solitaire::solvers::IPolicy::evaluate_move);

    py::class_<solitaire::solvers::CompleteDfsSolver, std::shared_ptr<solitaire::solvers::CompleteDfsSolver>>(m, "CompleteDfsSolver")
        .def(py::init<>())
        .def("solve", &solitaire::solvers::CompleteDfsSolver::solve,
             py::arg("initial"), py::arg("cfg") = solitaire::SolverConfig());

    py::class_<solitaire::solvers::HeuristicRunner>(m, "HeuristicRunner")
        .def(py::init<>())
        .def("run", &solitaire::solvers::HeuristicRunner::run,
             py::arg("initial"), py::arg("policy"), py::arg("cfg") = solitaire::SolverConfig());

    py::class_<solitaire::solvers::GreedyPolicy, solitaire::solvers::IPolicy,
               std::shared_ptr<solitaire::solvers::GreedyPolicy>>(m, "GreedyPolicy")
        .def(py::init<>())
        .def("choose_move", &solitaire::solvers::GreedyPolicy::choose_move);

    py::class_<solitaire::solvers::RandomPolicy, solitaire::solvers::IPolicy,
               std::shared_ptr<solitaire::solvers::RandomPolicy>>(m, "RandomPolicy")
        .def(py::init<unsigned int>(), py::arg("seed") = 0)
        .def("choose_move", &solitaire::solvers::RandomPolicy::choose_move);
}
