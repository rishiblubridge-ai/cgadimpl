#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>

#include "ad/ag_all.hpp"

using namespace ag;
using namespace OwnTensor;

// ========================================
// AG Framework MLP Benchmark
// ========================================

struct AGMLP {
    Value W1, W2, W3;
    
    AGMLP(int input_size, int hidden_size, int output_size) {
        Tensor W1_t = Tensor::zeros(Shape{{input_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W2_t = Tensor::zeros(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W3_t = Tensor::zeros(Shape{{hidden_size, output_size}}, TensorOptions().with_req_grad(true));
        
        W1 = make_tensor(W1_t, "W1");
        W2 = make_tensor(W2_t, "W2");
        W3 = make_tensor(W3_t, "W3");
    }
    
    Value forward(Value x) {
        auto h1 = sigmoid(l(x, W1));
        auto h2 = sigmoid(matmul(h1, W2));
        auto out = matmul(h2, W3);
        return out;
    }
};

// Load data from torch benchmark
bool load_benchmark_data(const std::string& filename, 
                         std::vector<float>& w1, std::vector<float>& w2, std::vector<float>& w3,
                         std::vector<float>& input, std::vector<float>& target) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        std::cerr << "Please run bench_Mlp first to generate the data file!" << std::endl;
        return false;
    }
    
    std::string line;
    
    // Read W1
    std::getline(file, line); // "W1"
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            float val;
            file >> val;
            w1.push_back(val);
        }
    }
    std::getline(file, line); // consume newline
    
    // Read W2
    std::getline(file, line); // "W2"
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            float val;
            file >> val;
            w2.push_back(val);
        }
    }
    std::getline(file, line); // consume newline
    
    // Read W3
    std::getline(file, line); // "W3"
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            float val;
            file >> val;
            w3.push_back(val);
        }
    }
    std::getline(file, line); // consume newline
    
    // Read INPUT
    std::getline(file, line); // "INPUT"
    for (int i = 0; i < 2; ++i) {
        float val;
        file >> val;
        input.push_back(val);
    }
    std::getline(file, line); // consume newline
    
    // Read TARGET
    std::getline(file, line); // "TARGET"
    for (int i = 0; i < 2; ++i) {
        float val;
        file >> val;
        target.push_back(val);
    }
    
    file.close();
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "AG Framework MLP Training Benchmark" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Configuration
    const int input_size = 2;
    const int hidden_size = 2;
    const int output_size = 2;
    const int batch_size = 1;
    const int num_iterations = 100;
    const float learning_rate = 0.1f;
    
    // Load data from torch benchmark
    std::vector<float> w1_data, w2_data, w3_data, input_data, target_data;
    if (!load_benchmark_data("torch_benchmark_data.txt", w1_data, w2_data, w3_data, input_data, target_data)) {
        return 1;
    }
    
    std::cout << "✓ Loaded initial data from torch_benchmark_data.txt\n" << std::endl;
    
    // Initialize model
    AGMLP model(input_size, hidden_size, output_size);
    
    // Copy weights (note: torch stores as [out, in], AG stores as [in, out])
    float* w1_ptr = model.W1.val().data<float>();
    float* w2_ptr = model.W2.val().data<float>();
    float* w3_ptr = model.W3.val().data<float>();
    
    // Transpose while copying (torch is row-major [out_features, in_features], we need [in, out])
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            w1_ptr[j * 2 + i] = w1_data[i * 2 + j];
            w2_ptr[j * 2 + i] = w2_data[i * 2 + j];
            w3_ptr[j * 2 + i] = w3_data[i * 2 + j];
        }
    }
    
    std::cout << "Initial Weights:" << std::endl;
    std::cout << "W1: [" << w1_ptr[0] << ", " << w1_ptr[1] << "; " << w1_ptr[2] << ", " << w1_ptr[3] << "]" << std::endl;
    std::cout << "W2: [" << w2_ptr[0] << ", " << w2_ptr[1] << "; " << w2_ptr[2] << ", " << w2_ptr[3] << "]" << std::endl;
    std::cout << "W3: [" << w3_ptr[0] << ", " << w3_ptr[1] << "; " << w3_ptr[2] << ", " << w3_ptr[3] << "]\n" << std::endl;
    
    // Create input and target
    Tensor input_t = Tensor::zeros(Shape{{batch_size, input_size}}, TensorOptions().with_req_grad(false));
    Tensor target_t = Tensor::zeros(Shape{{batch_size, output_size}}, TensorOptions().with_req_grad(false));
    
    std::memcpy(input_t.data<float>(), input_data.data(), input_data.size() * sizeof(float));
    std::memcpy(target_t.data<float>(), target_data.data(), target_data.size() * sizeof(float));
    
    Value input = make_tensor(input_t, "input");
    Value target = make_tensor(target_t, "target");
    
    std::cout << "Input: [" << input_data[0] << ", " << input_data[1] << "]" << std::endl;
    std::cout << "Target: [" << target_data[0] << ", " << target_data[1] << "]\n" << std::endl;
    
    // Training
    std::cout << "Training for " << num_iterations << " iterations..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        // Forward pass
        Value output = model.forward(input);
        
        // Compute MSE loss manually
        Value diff = output - target;
        Value squared = diff * diff;
        Value loss = sum(squared) * (1.0f / (batch_size * output_size));
        
        // Backward pass
        backward(loss);
        
        // SGD update
        float* w1_grad = model.W1.grad().data<float>();
        float* w2_grad = model.W2.grad().data<float>();
        float* w3_grad = model.W3.grad().data<float>();
        
        for (int j = 0; j < input_size * hidden_size; ++j) {
            w1_ptr[j] -= learning_rate * w1_grad[j];
        }
        for (int j = 0; j < hidden_size * hidden_size; ++j) {
            w2_ptr[j] -= learning_rate * w2_grad[j];
        }
        for (int j = 0; j < hidden_size * output_size; ++j) {
            w3_ptr[j] -= learning_rate * w3_grad[j];
        }
        
        // Zero gradients
        std::memset(w1_grad, 0, input_size * hidden_size * sizeof(float));
        std::memset(w2_grad, 0, hidden_size * hidden_size * sizeof(float));
        std::memset(w3_grad, 0, hidden_size * output_size * sizeof(float));
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Iter [" << (i + 1) << "/" << num_iterations 
                      << "], Loss: " << loss.val().data<float>()[0] << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double training_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // Final results
    Value final_output = model.forward(input);
    Value final_diff = final_output - target;
    Value final_squared = final_diff * final_diff;
    Value final_loss = sum(final_squared) * (1.0f / (batch_size * output_size));
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Training Time: " << training_time << " ms\n" << std::endl;
    
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Final Loss: " << final_loss.val().data<float>()[0] << "\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(6);
    const float* out_ptr = final_output.val().data<float>();
    std::cout << "Final Output: [" << out_ptr[0] << ", " << out_ptr[1] << "]\n" << std::endl;
    
    std::cout << "Final Weights:" << std::endl;
    std::cout << "W1: [" << w1_ptr[0] << ", " << w1_ptr[1] << "; " << w1_ptr[2] << ", " << w1_ptr[3] << "]" << std::endl;
    std::cout << "W2: [" << w2_ptr[0] << ", " << w2_ptr[1] << "; " << w2_ptr[2] << ", " << w2_ptr[3] << "]" << std::endl;
    std::cout << "W3: [" << w3_ptr[0] << ", " << w3_ptr[1] << "; " << w3_ptr[2] << ", " << w3_ptr[3] << "]" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Compare with bench_Mlp results!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
