#include <chrono>
#include <iostream>
#include <iomanip>
#include <torch/torch.h>

// Benchmark: Isolate LibTorch's graph traversal overhead
// By measuring: (Forward+Backward) - (Forward only)

struct TorchMLP : torch::nn::Module {
    TorchMLP(int input_size, int h1, int h2, int h3, int h4, int output_size) {
        fc1 = register_module("fc1", torch::nn::Linear(input_size, h1));
        fc2 = register_module("fc2", torch::nn::Linear(h1, h2));
        fc3 = register_module("fc3", torch::nn::Linear(h2, h3));
        fc4 = register_module("fc4", torch::nn::Linear(h3, h4));
        fc5 = register_module("fc5", torch::nn::Linear(h4, output_size));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::gelu(fc1->forward(x));
        x = torch::silu(fc2->forward(x));
        x = torch::leaky_relu(fc3->forward(x), 0.1);
        x = torch::softplus(fc4->forward(x));
        x = fc5->forward(x);
        return x;
    }

    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc4{nullptr}, fc5{nullptr};
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LibTorch Graph Traversal Overhead" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const int batch_size = 8;
    const int input_size = 16;
    const int h1 = 64, h2 = 64, h3 = 32, h4 = 32;
    const int output_size = 10;
    const int num_iterations = 10000;
    
    torch::manual_seed(42);
    
    auto model = std::make_shared<TorchMLP>(input_size, h1, h2, h3, h4, output_size);
    torch::Tensor input = torch::randn({batch_size, input_size});
    torch::Tensor target = torch::randn({batch_size, output_size});
    torch::nn::MSELoss criterion;
    
    // Warmup
    for (int i = 0; i < 100; ++i) {
        auto output = model->forward(input);
        auto loss = criterion(output, target);
        loss.backward();
        model->zero_grad();
    }
    
    // Benchmark 1: Forward only (no graph traversal)
    std::cout << "Measuring forward pass only..." << std::endl;
    auto start_fwd = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        torch::NoGradGuard no_grad;  // Disable autograd
        auto output = model->forward(input);
        auto loss = criterion(output, target);
    }
    
    auto end_fwd = std::chrono::high_resolution_clock::now();
    double fwd_time = std::chrono::duration<double, std::milli>(end_fwd - start_fwd).count();
    
    // Benchmark 2: Forward + Backward (includes graph traversal)
    std::cout << "Measuring forward + backward..." << std::endl;
    auto start_bwd = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        auto output = model->forward(input);
        auto loss = criterion(output, target);
        loss.backward();  // This includes tape replay (graph traversal)
        model->zero_grad();
    }
    
    auto end_bwd = std::chrono::high_resolution_clock::now();
    double fwd_bwd_time = std::chrono::duration<double, std::milli>(end_bwd - start_bwd).count();
    
    // Calculate graph traversal overhead
    double traversal_time = fwd_bwd_time - fwd_time;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS (" << num_iterations << " iterations)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Forward only:        " << fwd_time << " ms" << std::endl;
    std::cout << "Forward + Backward:  " << fwd_bwd_time << " ms" << std::endl;
    std::cout << "Graph traversal:     " << traversal_time << " ms" << std::endl;
    
    std::cout << "\nPer iteration:" << std::endl;
    std::cout << "Forward only:        " << (fwd_time / num_iterations) << " ms" << std::endl;
    std::cout << "Forward + Backward:  " << (fwd_bwd_time / num_iterations) << " ms" << std::endl;
    std::cout << "Graph traversal:     " << (traversal_time / num_iterations) << " ms" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Run bench_topo_ag to compare!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
