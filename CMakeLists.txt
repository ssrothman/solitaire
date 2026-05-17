cmake_minimum_required(VERSION 3.24)
project(solitaire LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build options
option(BUILD_TESTS "Build C++ unit tests" ON)
option(BUILD_PYTHON "Build Python bindings" ON)
option(BUILD_BENCHMARKS "Build C++ performance benchmarks" OFF)
option(ENABLE_CALLGRIND_SUPPORT "Build with debug-friendly flags for Valgrind/Callgrind" OFF)

if(ENABLE_CALLGRIND_SUPPORT)
    message(STATUS "Enabling Callgrind-friendly compiler flags")

    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    endif()

    if(MSVC)
        add_compile_options(/Zi /Od /Ob0 /Oy-)
        add_link_options(/DEBUG)
    else()
        add_compile_options(-O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-inline)
    endif()
endif()

# Include CMake utilities
include(GNUInstallDirs)

# Main solitaire library
add_library(solitaire_core STATIC)
set_target_properties(solitaire_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(solitaire_core PRIVATE
    src/solitaire/types.cpp
    src/solitaire/game_state.cpp
    src/solitaire/shuffle.cpp
    src/solitaire/solvers/solver.cpp
    src/solitaire/solvers/complete_dfs.cpp
    src/solitaire/solvers/pruning.cpp
    src/solitaire/solvers/greedy_policy.cpp
    src/solitaire/solvers/random_policy.cpp
    src/solitaire/util/move_notation.cpp
    src/solitaire/util/move_equivalence.cpp
    src/solitaire/util/move_analysis.cpp
)

target_include_directories(solitaire_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 QUIET)
    
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found; skipping C++ tests")
        set(BUILD_TESTS OFF)
    else()
        add_executable(test_game_rules tests/cpp/test_game_rules.cpp)
        target_link_libraries(test_game_rules PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME GameRules COMMAND test_game_rules)
        
        add_executable(test_state_invariants tests/cpp/test_state_invariants.cpp)
        target_link_libraries(test_state_invariants PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME StateInvariants COMMAND test_state_invariants)
        
        add_executable(test_complete_dfs tests/cpp/test_complete_dfs.cpp)
        target_link_libraries(test_complete_dfs PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME CompleteDfs COMMAND test_complete_dfs)
        
        add_executable(test_policies tests/cpp/test_policies.cpp)
        target_link_libraries(test_policies PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME Policies COMMAND test_policies)

        add_executable(test_move_notation tests/cpp/test_move_notation.cpp)
        target_link_libraries(test_move_notation PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME MoveNotation COMMAND test_move_notation)

        add_executable(test_move_equivalence tests/cpp/test_move_equivalence.cpp)
        target_link_libraries(test_move_equivalence PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME MoveEquivalence COMMAND test_move_equivalence)

        add_executable(test_move_analysis tests/cpp/test_move_analysis.cpp)
        target_link_libraries(test_move_analysis PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME MoveAnalysis COMMAND test_move_analysis)

        add_executable(test_pruning tests/cpp/test_pruning.cpp)
        target_link_libraries(test_pruning PRIVATE solitaire_core Catch2::Catch2WithMain)
        add_test(NAME Pruning COMMAND test_pruning)
    endif()
endif()

# Python bindings
if(BUILD_PYTHON)
    find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)
    find_package(pybind11 REQUIRED)
    
    pybind11_add_module(_solitaire_binding bindings/module.cpp)
    target_link_libraries(_solitaire_binding PRIVATE solitaire_core)
    
    # Install the Python package layout into the wheel/site-packages tree.
    set_target_properties(_solitaire_binding PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/solitaire"
    )
    
    install(TARGETS _solitaire_binding
        LIBRARY DESTINATION solitaire
    )

    install(FILES
        python/solitaire/__init__.py
        DESTINATION solitaire
    )

    install(FILES
        python/solitaire/ui/__init__.py
        python/solitaire/ui/cli.py
        python/solitaire/ui/main.py
        python/solitaire/ui/__main__.py
        DESTINATION solitaire/ui
    )
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_executable(benchmark_pruning benchmarks/cpp/benchmark_pruning.cpp)
    target_link_libraries(benchmark_pruning PRIVATE solitaire_core)

    add_executable(benchmark_move_generation benchmarks/cpp/benchmark_move_generation.cpp)
    target_link_libraries(benchmark_move_generation PRIVATE solitaire_core)

    add_executable(benchmark_completeDFS benchmarks/cpp/benchmark_completeDFS.cpp)
    target_link_libraries(benchmark_completeDFS PRIVATE solitaire_core)

endif()

# Install headers
install(DIRECTORY include/solitaire
    DESTINATION include
)

# Export targets
install(TARGETS solitaire_core
    EXPORT solitaire_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(EXPORT solitaire_targets
    FILE solitaire-targets.cmake
    NAMESPACE solitaire::
    DESTINATION lib/cmake/solitaire
)
