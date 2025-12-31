#include <chrono>
#include <iostream>
#include <iomanip>
#include "ad/ag_all.hpp"

using namespace ag;
using namespace OwnTensor;

// Benchmark: Isolate AG's topo_from overhead
// By measuring: (Forward+Backward) - (Forward only)

struct AGMLP {
    Value W1, b1, W2, b2, W3, b3, W4, b4, W5, b5;
    
    AGMLP(int input_size, int h1, int h2, int h3, int h4, int output_size) {
        auto opts = TensorOptions().with_req_grad(true);
        W1 = make_tensor(Tensor::randn(Shape{{input_size, h1}}, opts), "W1");
        b1 = make_tensor(Tensor::zeros(Shape{{1, h1}}, opts), "b1");
        W2 = make_tensor(Tensor::randn(Shape{{h1, h2}}, opts), "W2");
        b2 = make_tensor(Tensor::zeros(Shape{{1, h2}}, opts), "b2");
        W3 = make_tensor(Tensor::randn(Shape{{h2, h3}}, opts), "W3");
        b3 = make_tensor(Tensor::zeros(Shape{{1, h3}}, opts), "b3");
        W4 = make_tensor(Tensor::randn(Shape{{h3, h4}}, opts), "W4");
        b4 = make_tensor(Tensor::zeros(Shape{{1, h4}}, opts), "b4");
        W5 = make_tensor(Tensor::randn(Shape{{h4, output_size}}, opts), "W5");
        b5 = make_tensor(Tensor::zeros(Shape{{1, output_size}}, opts), "b5");
    }
    
    Value forward(Value x) {
        auto h1 = gelu(matmul(x, W1) + b1);
        auto h2 = silu(matmul(h1, W2) + b2);
        auto h3 = leaky_relu(matmul(h2, W3) + b3, 0.1f);
        auto h4 = softplus(matmul(h3, W4) + b4);
        auto out = matmul(h4, W5) + b5;
        return out;
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "AG Framework topo_from Overhead" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const int batch_size = 8;
    const int input_size = 16;
    const int h1 = 64, h2 = 64, h3 = 32, h4 = 32;
    const int output_size = 10;
    const int num_iterations = 10000;
    
    AGMLP model(input_size, h1, h2, h3, h4, output_size);
    
    Tensor input_t = Tensor::randn(Shape{{batch_size, input_size}}, TensorOptions().with_req_grad(false));
    Tensor target_t = Tensor::randn(Shape{{batch_size, output_size}}, TensorOptions().with_req_grad(false));
    
    Value input = make_tensor(input_t, "input");
    Value target = make_tensor(target_t, "target");
    
    // Warmup
    for (int i = 0; i < 100; ++i) {
        Value output = model.forward(input);
        Value diff = output - target;
        Value loss = sum(diff * diff) * (1.0f / (batch_size * output_size));
        backward(loss);
        zero_grad(loss);
    }
    
    // Benchmark 1: Forward only (no topo_from)
    std::cout << "Measuring forward pass only..." << std::endl;
    auto start_fwd = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        Value output = model.forward(input);
        Value diff = output - target;
        Value loss = sum(diff * diff) * (1.0f / (batch_size * output_size));
        // No backward - no topo_from called
    }
    
    auto end_fwd = std::chrono::high_resolution_clock::now();
    double fwd_time = std::chrono::duration<double, std::milli>(end_fwd - start_fwd).count();
    
    // Benchmark 2: Forward + Backward (includes topo_from)
    std::cout << "Measuring forward + backward..." << std::endl;
    auto start_bwd = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        Value output = model.forward(input);
        Value diff = output - target;
        Value loss = sum(diff * diff) * (1.0f / (batch_size * output_size));
        backward(loss);  // This calls topo_from()
        zero_grad(loss);
    }
    
    auto end_bwd = std::chrono::high_resolution_clock::now();
    double fwd_bwd_time = std::chrono::duration<double, std::milli>(end_bwd - start_bwd).count();
    
    // Calculate topo_from overhead
    double topo_time = fwd_bwd_time - fwd_time;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS (" << num_iterations << " iterations)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Forward only:        " << fwd_time << " ms" << std::endl;
    std::cout << "Forward + Backward:  " << fwd_bwd_time << " ms" << std::endl;
    std::cout << "topo_from overhead:  " << topo_time << " ms" << std::endl;
    
    std::cout << "\nPer iteration:" << std::endl;
    std::cout << "Forward only:        " << (fwd_time / num_iterations) << " ms" << std::endl;
    std::cout << "Forward + Backward:  " << (fwd_bwd_time / num_iterations) << " ms" << std::endl;
    std::cout << "topo_from overhead:  " << (topo_time / num_iterations) << " ms" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Compare with bench_topo_torch!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
