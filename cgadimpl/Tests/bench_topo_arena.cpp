#include <iostream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <functional>
#include "ad/ag_all.hpp"

using namespace ag;

// ========================================
// BASELINE: Standard Heap Implementation
// ========================================
// This is what topo_from would look like WITHOUT arena optimization
static std::vector<Node*> topo_from_heap(Node* root) {
    std::vector<Node*> order;
    order.reserve(256);
    
    std::unordered_set<Node*> vis;
    vis.reserve(256);
    
    std::function<void(Node*)> dfs = [&](Node* n) {
        if (!n || vis.count(n)) return;
        vis.insert(n);
        for (auto& p : n->inputs) dfs(p.get());
        order.push_back(n);
    };
    
    dfs(root);
    return order;
}

// ========================================
// Graph Building Utilities
// ========================================

// Build a simple chain: x -> op1 -> op2 -> ... -> opN
Value build_chain(int depth, int hidden_size) {
    Tensor X = Tensor::randn(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(false));
    Tensor W = Tensor::randn(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
    
    auto x = make_tensor(X, "x");
    auto w = make_tensor(W, "w");
    
    auto layer = x;
    for (int i = 0; i < depth; i++) {
        layer = relu(matmul(layer, w));
    }
    return layer;
}

// Build a multi-branch graph (parallel branches that merge)
Value build_multi_branch(int num_branches, int depth_per_branch, int hidden_size) {
    std::vector<Value> branches;
    
    for (int b = 0; b < num_branches; b++) {
        Tensor X = Tensor::randn(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(false));
        Tensor W = Tensor::randn(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
        
        auto x = make_tensor(X, ("x" + std::to_string(b)).c_str());
        auto w = make_tensor(W, ("w" + std::to_string(b)).c_str());
        
        auto layer = x;
        for (int i = 0; i < depth_per_branch; i++) {
            layer = relu(matmul(layer, w));
        }
        branches.push_back(layer);
    }
    
    // Merge all branches
    auto result = branches[0];
    for (size_t i = 1; i < branches.size(); i++) {
        result = result + branches[i];
    }
    return result;
}

// Build a diamond pattern (creates many shared dependencies)
Value build_diamond(int num_diamonds, int hidden_size) {
    Tensor X = Tensor::randn(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(false));
    Tensor W = Tensor::randn(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
    
    auto x = make_tensor(X, "x");
    auto w = make_tensor(W, "w");
    
    auto current = x;
    for (int d = 0; d < num_diamonds; d++) {
        auto left = relu(matmul(current, w));
        auto right = relu(matmul(current, w));
        current = left + right;
    }
    return current;
}

// ========================================
// Timing Utilities
// ========================================

using clock_type = std::chrono::high_resolution_clock;

struct BenchmarkResult {
    double median_ms;
    double min_ms;
    double max_ms;
    size_t num_nodes;
};

template<typename Func>
BenchmarkResult benchmark_topo(const std::string& name, Node* root, Func topo_func, int iterations = 20) {
    std::vector<double> times;
    times.reserve(iterations);
    
    // Warm-up runs
    for (int i = 0; i < 3; i++) {
        auto result = topo_func(root);
        (void)result; // Prevent optimization
    }
    
    // Actual benchmark runs
    for (int i = 0; i < iterations; i++) {
        auto t0 = clock_type::now();
        auto result = topo_func(root);
        auto t1 = clock_type::now();
        
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }
    
    std::sort(times.begin(), times.end());
    
    BenchmarkResult res;
    res.median_ms = times[times.size() / 2];
    res.min_ms = times.front();
    res.max_ms = times.back();
    res.num_nodes = topo_func(root).size();
    
    return res;
}

// ========================================
// Correctness Verification
// ========================================

bool verify_correctness(Node* root) {
    auto heap_order = topo_from_heap(root);
    auto arena_order = topo_from(root);
    
    if (heap_order.size() != arena_order.size()) {
        std::cerr << "ERROR: Size mismatch! Heap: " << heap_order.size() 
                  << ", Arena: " << arena_order.size() << std::endl;
        return false;
    }
    
    // Both should produce valid topological orderings
    // (exact order may differ, but both should be valid)
    std::unordered_set<Node*> heap_set(heap_order.begin(), heap_order.end());
    std::unordered_set<Node*> arena_set(arena_order.begin(), arena_order.end());
    
    if (heap_set != arena_set) {
        std::cerr << "ERROR: Node sets don't match!" << std::endl;
        return false;
    }
    
    return true;
}

// ========================================
// Main Benchmark Suite
// ========================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ARENA ALLOCATION BENCHMARK" << std::endl;
    std::cout << "Testing topo_from Performance" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    struct TestCase {
        std::string name;
        std::string description;
        Value graph;
    };
    
    std::vector<TestCase> test_cases;
    
    std::cout << "Building test graphs..." << std::endl;
    
    // Test 1: Small chain
    test_cases.push_back({
        "Small Chain",
        "100 sequential operations",
        build_chain(100, 64)
    });
    std::cout << "  ✓ Small chain (100 nodes)" << std::endl;
    
    // Test 2: Medium chain
    test_cases.push_back({
        "Medium Chain",
        "500 sequential operations",
        build_chain(500, 64)
    });
    std::cout << "  ✓ Medium chain (500 nodes)" << std::endl;
    
    // Test 3: Large chain
    test_cases.push_back({
        "Large Chain",
        "2000 sequential operations",
        build_chain(2000, 64)
    });
    std::cout << "  ✓ Large chain (2000 nodes)" << std::endl;
    
    // Test 4: Multi-branch (parallel structure)
    test_cases.push_back({
        "Multi-Branch",
        "4 branches × 250 ops each",
        build_multi_branch(4, 250, 64)
    });
    std::cout << "  ✓ Multi-branch graph (1000+ nodes)" << std::endl;
    
    // Test 5: Diamond pattern (shared dependencies)
    test_cases.push_back({
        "Diamond Pattern",
        "10 diamond structures",
        build_diamond(10, 64)
    });
    std::cout << "  ✓ Diamond pattern graph" << std::endl;
    
    // Test 6: Very large chain
    test_cases.push_back({
        "Very Large Chain",
        "5000 sequential operations",
        build_chain(5000, 64)
    });
    std::cout << "  ✓ Very large chain (5000 nodes)" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Benchmarks..." << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Results table
    std::cout << std::left << std::setw(20) << "Test Case"
              << std::right << std::setw(10) << "Nodes"
              << std::setw(12) << "Heap (ms)"
              << std::setw(12) << "Arena (ms)"
              << std::setw(12) << "Speedup"
              << std::endl;
    std::cout << std::string(66, '-') << std::endl;
    
    double total_speedup = 0.0;
    int num_tests = 0;
    
    for (auto& test : test_cases) {
        Node* root = test.graph.node.get();
        
        // Verify correctness first
        if (!verify_correctness(root)) {
            std::cerr << "FAILED: " << test.name << " - Correctness check failed!" << std::endl;
            continue;
        }
        
        // Benchmark heap version
        auto heap_result = benchmark_topo("Heap", root, topo_from_heap, 15);
        
        // Benchmark arena version (use lambda to resolve overload)
        auto arena_result = benchmark_topo("Arena", root, 
            [](Node* n) { return topo_from(n); }, 15);
        
        double speedup = heap_result.median_ms / arena_result.median_ms;
        total_speedup += speedup;
        num_tests++;
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::left << std::setw(20) << test.name
                  << std::right << std::setw(10) << heap_result.num_nodes
                  << std::setw(12) << heap_result.median_ms
                  << std::setw(12) << arena_result.median_ms
                  << std::setw(11) << speedup << "x"
                  << std::endl;
    }
    
    std::cout << std::string(66, '-') << std::endl;
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    
    double avg_speedup = total_speedup / num_tests;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average Speedup:  " << avg_speedup << "x" << std::endl;
    
    if (avg_speedup > 2.0) {
        std::cout << "\nExcellent! Arena allocation provides significant" << std::endl;
        std::cout << "   performance improvements (>2x speedup)!" << std::endl;
    } else if (avg_speedup > 1.3) {
        std::cout << "\nGood! Arena allocation provides measurable" << std::endl;
        std::cout << "   performance improvements." << std::endl;
    } else {
        std::cout << "\nℹModest speedup. Arena benefits are most visible" << std::endl;
        std::cout << "   with larger graphs and many allocations." << std::endl;
    }
    
    std::cout << "\nKey Insights:" << std::endl;
    std::cout << "   • Arena uses bump allocation (O(1) per alloc)" << std::endl;
    std::cout << "   • Heap uses general allocator (O(log n) per alloc)" << std::endl;
    std::cout << "   • Larger graphs = more allocations = bigger speedup" << std::endl;
    std::cout << "   • Arena eliminates fragmentation overhead" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
} 
