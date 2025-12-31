#include <chrono>
#include <iostream>
#include <iomanip>
#include <torch/torch.h>

// Benchmark: LibTorch backward pass overhead (includes graph traversal)
// This measures the time spent in backward(), which includes:
// - Graph traversal (tape replay)
// - Gradient computation
// - Memory operations

struct TorchMLP : torch::nn::Module {
    TorchMLP(int input_size, int hidden_size, int output_size) {
        fc1 = register_module("fc1", torch::nn::Linear(input_size, hidden_size));
        fc2 = register_module("fc2", torch::nn::Linear(hidden_size, hidden_size));
        fc3 = register_module("fc3", torch::nn::Linear(hidden_size, output_size));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::sigmoid(fc1->forward(x));
        x = torch::sigmoid(fc2->forward(x));
        x = fc3->forward(x);
        return x;
    }

    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr};
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LibTorch Backward Pass Overhead Benchmark" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const int input_size = 2;
    const int hidden_size = 2;
    const int output_size = 2;
    const int batch_size = 1;
    const int num_iterations = 1000;  // More iterations for better timing
    
    torch::manual_seed(42);
    
    auto model = std::make_shared<TorchMLP>(input_size, hidden_size, output_size);
    torch::Tensor input = torch::randn({batch_size, input_size}, torch::requires_grad(false));
    torch::Tensor target = torch::randn({batch_size, output_size});
    
    torch::nn::MSELoss criterion;
    
    std::cout << "Warming up..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        torch::Tensor output = model->forward(input);
        torch::Tensor loss = criterion(output, target);
        loss.backward();
        model->zero_grad();
    }
    
    std::cout << "Running " << num_iterations << " backward passes..." << std::endl;
    
    // Measure backward pass time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        torch::Tensor output = model->forward(input);
        torch::Tensor loss = criterion(output, target);
        loss.backward();  // This includes graph traversal
        model->zero_grad();
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
    std::cout << "Run bench_backward_ag to compare!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
