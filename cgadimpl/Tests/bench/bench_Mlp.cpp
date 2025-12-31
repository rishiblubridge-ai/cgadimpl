#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <torch/torch.h> 

struct TorchMLP : torch::nn::module {
    TorchMLP(int input_size, int hidden_size, int output_size) {
        fc1 = register_module ("fc1", torch::nn::Linear(input_size, )) 
    }
}