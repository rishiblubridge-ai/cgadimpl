#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <torch/torch.h>

// ========================================
// LibTorch MLP Benchmark (with Bias)
// ========================================

struct TorchMLP : torch::nn::Module {
    TorchMLP(int input_size, int hidden_size, int output_size) {
        // Create layers WITH bias
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
    std::cout << "LibTorch MLP Training Benchmark (with Bias)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Configuration
    const int input_size = 2;
    const int hidden_size = 2;
    const int output_size = 2;
    const int batch_size = 1;
    const int num_iterations = 100;
    const float learning_rate = 0.1f;
    
    // Set random seed for reproducibility
    torch::manual_seed(42);
    
    // Initialize model
    auto model = std::make_shared<TorchMLP>(input_size, hidden_size, output_size);
    
    // Initialize weights and biases
    model->fc1->weight.data().uniform_(-0.5, 0.5);
    model->fc1->bias.data().uniform_(-0.5, 0.5);
    model->fc2->weight.data().uniform_(-0.5, 0.5);
    model->fc2->bias.data().uniform_(-0.5, 0.5);
    model->fc3->weight.data().uniform_(-0.5, 0.5);
    model->fc3->bias.data().uniform_(-0.5, 0.5);
    
    std::cout << "Initial Weights and Biases:" << std::endl;
    std::cout << "W1:\n" << model->fc1->weight.data() << std::endl;
    std::cout << "b1: " << model->fc1->bias.data() << std::endl;
    std::cout << "W2:\n" << model->fc2->weight.data() << std::endl;
    std::cout << "b2: " << model->fc2->bias.data() << std::endl;
    std::cout << "W3:\n" << model->fc3->weight.data() << std::endl; 
    std::cout << "b3: " << model->fc3->bias.data() << "\n" << std::endl; 
    
    // Create input and target
    torch::Tensor input = torch::randn({batch_size, input_size});
    torch::Tensor target = torch::randn({batch_size, output_size});
    
    std::cout << "Input:\n" << input << std::endl;
    std::cout << "Target:\n" << target << "\n" << std::endl;
    
    // Save initial data to file for AG benchmark
    std::ofstream data_file("torch_benchmark_data.txt");
    data_file << std::setprecision(10);
    
    // Save weights and biases
    auto save_matrix = [&](const std::string& name, const torch::Tensor& t) {
        data_file << name << "\n";
        for (int i = 0; i < t.size(0); ++i) {
            for (int j = 0; j < t.size(1); ++j) {
                data_file << t[i][j].item<float>() << " ";
            }
            data_file << "\n";
        }
    };
    
    auto save_vector = [&](const std::string& name, const torch::Tensor& t) {
        data_file << name << "\n";
        for (int i = 0; i < t.size(0); ++i) {
            data_file << t[i].item<float>() << " ";
        }
        data_file << "\n";
    };
    
    save_matrix("W1", model->fc1->weight.data());
    save_vector("b1", model->fc1->bias.data());
    save_matrix("W2", model->fc2->weight.data());
    save_vector("b2", model->fc2->bias.data());
    save_matrix("W3", model->fc3->weight.data());
    save_vector("b3", model->fc3->bias.data());
    save_vector("INPUT", input.view({-1}));
    save_vector("TARGET", target.view({-1}));
    
    data_file.close();
    std::cout << "✓ Saved initial data to torch_benchmark_data.txt\n" << std::endl;
    
    // Training
    torch::optim::SGD optimizer(model->parameters(), learning_rate);
    torch::nn::MSELoss criterion;
    
    std::cout << "Training for " << num_iterations << " iterations..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        optimizer.zero_grad();
        torch::Tensor output = model->forward(input);
        torch::Tensor loss = criterion(output, target);
        loss.backward();
        optimizer.step();
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Iter [" << (i + 1) << "/" << num_iterations 
                      << "], Loss: " << loss.item<double>() << std::endl;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double training_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // Final results
    torch::Tensor final_output = model->forward(input);
    torch::Tensor final_loss = criterion(final_output, target);
    
    // Save final gradients for comparison
    std::ofstream grad_file("torch_gradients.txt");
    grad_file << std::setprecision(10);
    save_matrix("W1_grad", model->fc1->weight.grad());
    save_vector("b1_grad", model->fc1->bias.grad());
    save_matrix("W2_grad", model->fc2->weight.grad());
    save_vector("b2_grad", model->fc2->bias.grad());
    save_matrix("W3_grad", model->fc3->weight.grad());
    save_vector("b3_grad", model->fc3->bias.grad());
    grad_file.close();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Training Time: " << training_time << " ms\n" << std::endl;
    
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Final Loss: " << final_loss.item<double>() << "\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Final Output:\n" << final_output << "\n" << std::endl;
    
    std::cout << "Final Weights and Biases:" << std::endl;
    std::cout << "W1:\n" << model->fc1->weight.data() << std::endl;
    std::cout << "b1: " << model->fc1->bias.data() << std::endl;
    std::cout << "W2:\n" << model->fc2->weight.data() << std::endl;
    std::cout << "b2: " << model->fc2->bias.data() << std::endl;
    std::cout << "W3:\n" << model->fc3->weight.data() << std::endl;
    std::cout << "b3: " << model->fc3->bias.data() << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "GRADIENT ACCURACY" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::cout << "Final Gradients (LibTorch):" << std::endl;
    std::cout << "W1_grad:\n" << model->fc1->weight.grad() << std::endl;
    std::cout << "b1_grad: " << model->fc1->bias.grad() << std::endl;
    std::cout << "W2_grad:\n" << model->fc2->weight.grad() << std::endl;
    std::cout << "b2_grad: " << model->fc2->bias.grad() << std::endl;
    std::cout << "W3_grad:\n" << model->fc3->weight.grad() << std::endl;
    std::cout << "b3_grad: " << model->fc3->bias.grad() << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Run bench_Mlp_ag to compare with AG!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
