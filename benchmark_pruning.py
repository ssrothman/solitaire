#!/usr/bin/env python3
"""
Benchmark script to demonstrate move-equivalence pruning impact
"""

import sys
import time
import solitaire

def benchmark_solver(seed: int, enable_equivalence: bool = True, enable_no_op: bool = False):
    """Solve a game and report pruning statistics"""
    state = solitaire.new_game(seed=seed)
    
    cfg = solitaire.SolverConfig()
    cfg.max_nodes = 1_000_000
    cfg.enable_move_equivalence_pruning = enable_equivalence
    cfg.enable_no_op_pruning = enable_no_op
    
    start = time.time()
    result = solitaire.solve_game(state, cfg)
    elapsed = time.time() - start
    
    return {
        'solvable': result.solvable,
        'nodes': result.stats.nodes_explored,
        'time': elapsed,
        'max_depth': result.stats.max_depth,
    }

def main():
    print("Move-Equivalence Pruning Benchmark")
    print("=" * 60)
    
    # Test a few representative seeds
    test_seeds = [1, 42, 123, 456, 789]
    
    print(f"\n{'Seed':<8} {'Without Pruning':<25} {'With Pruning':<25} {'Speedup':<10}")
    print(f"{'':8} {'Nodes':<12} {'Time (s)':<12} {'Nodes':<12} {'Time (s)':<12} {'(x)':<10}")
    print("-" * 90)
    
    for seed in test_seeds:
        try:
            # Benchmark without pruning
            result_unpruned = benchmark_solver(seed, enable_equivalence=False)
            
            # Benchmark with pruning
            result_pruned = benchmark_solver(seed, enable_equivalence=True)
            
            nodes_reduction = (1.0 - result_pruned['nodes'] / max(result_unpruned['nodes'], 1)) * 100
            speedup = result_unpruned['time'] / max(result_pruned['time'], 0.001)
            
            print(f"{seed:<8} {result_unpruned['nodes']:<12} {result_unpruned['time']:<12.4f} "
                  f"{result_pruned['nodes']:<12} {result_pruned['time']:<12.4f} {speedup:<10.2f}x")
        except Exception as e:
            print(f"{seed:<8} ERROR: {str(e)[:50]}")
    
    print("\n" + "=" * 60)
    print("Note: Tests may hit max_nodes limit; compare with and without pruning")

if __name__ == "__main__":
    main()
