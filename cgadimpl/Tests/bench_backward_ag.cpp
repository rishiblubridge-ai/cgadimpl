#include <chrono>
#include <iostream>
#include <iomanip>
#include "ad/ag_all.hpp"

using namespace ag;
using namespace OwnTensor;

// Benchmark: AG Framework backward pass overhead (includes topo_from)
// This measures the time spent in backward(), which includes:
// - topo_from() graph traversal
// - Gradient computation
// - Memory operations

struct AGMLP {
    Value W1, W2, W3;
    Value b1, b2, b3;
    
    AGMLP(int input_size, int hidden_size, int output_size) {
        Tensor W1_t = Tensor::randn(Shape{{input_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W2_t = Tensor::randn(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W3_t = Tensor::randn(Shape{{hidden_size, output_size}}, TensorOptions().with_req_grad(true));
        Tensor b1_t = Tensor::randn(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor b2_t = Tensor::randn(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor b3_t = Tensor::randn(Shape{{1, output_size}}, TensorOptions().with_req_grad(true));
        
        W1 = make_tensor(W1_t, "W1");
        W2 = make_tensor(W2_t, "W2");
        W3 = make_tensor(W3_t, "W3");
        b1 = make_tensor(b1_t, "b1");
        b2 = make_tensor(b2_t, "b2");
        b3 = make_tensor(b3_t, "b3");
    }
    
    Value forward(Value x) {
        auto h1 = sigmoid(matmul(x, W1) + b1);
        auto h2 = sigmoid(matmul(h1, W2) + b2);
        auto out = matmul(h2, W3) + b3;
        return out;
    }
    
    void zero_grad() {
        std::memset(W1.grad().data<float>(), 0, 4 * sizeof(float));
        std::memset(W2.grad().data<float>(), 0, 4 * sizeof(float));
        std::memset(W3.grad().data<float>(), 0, 4 * sizeof(float));
        std::memset(b1.grad().data<float>(), 0, 2 * sizeof(float));
        std::memset(b2.grad().data<float>(), 0, 2 * sizeof(float));
        std::memset(b3.grad().data<float>(), 0, 2 * sizeof(float));
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "AG Framework Backward Pass Overhead Benchmark" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const int input_size = 2;
    const int hidden_size = 2;
    const int output_size = 2;
    const int batch_size = 1;
    const int num_iterations = 1000;  // Same as LibTorch benchmark
    
    AGMLP model(input_size, hidden_size, output_size);
    
    Tensor input_t = Tensor::randn(Shape{{batch_size, input_size}}, TensorOptions().with_req_grad(false));
    Tensor target_t = Tensor::randn(Shape{{batch_size, output_size}}, TensorOptions().with_req_grad(false));
    
    Value input = make_tensor(input_t, "input");
    Value target = make_tensor(target_t, "target");
    
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        Value output = model.forward(input);
        Value diff = output - target;
        Value loss = sum(diff * diff) * (1.0f / (batch_size * output_size));
        backward(loss);  // This includes topo_from()
        model.zero_grad();
    }
    
    std::cout << "Running " << num_iterations << " backward passes..." << std::endl;
    
    // Measure backward pass time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        Value output = model.forward(input);
        Value diff = output - target;
        Value loss = sum(diff * diff) * (1.0f / (batch_size * output_size));
        backward(loss);  // This includes topo_from()
        model.zero_grad();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total time: " << total_time << " ms" << std::endl;
    std::cout << "Average per iteration: " << (total_time / num_iterations) << " ms" << std::endl;
    std::cout << "Iterations per second: " << (num_iterations / (total_time / 1000.0)) << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Compare with bench_backward_torch!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
