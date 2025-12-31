#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

#include "ad/ag_all.hpp"

using namespace ag;
using namespace OwnTensor;

// ========================================
// NODISCARD TEST: Intentionally Ignoring Return Values
// ========================================
// This file tests if AG_NODISCARD (which should be [[nodiscard]])
// properly generates compiler warnings when Value return values are ignored.

struct AGMLP {
    Value W1, W2, W3;
    Value b1, b2, b3;
    
    AGMLP(int input_size, int hidden_size, int output_size) {
        Tensor W1_t = Tensor::zeros(Shape{{input_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W2_t = Tensor::zeros(Shape{{hidden_size, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor W3_t = Tensor::zeros(Shape{{hidden_size, output_size}}, TensorOptions().with_req_grad(true));
        Tensor b1_t = Tensor::zeros(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor b2_t = Tensor::zeros(Shape{{1, hidden_size}}, TensorOptions().with_req_grad(true));
        Tensor b3_t = Tensor::zeros(Shape{{1, output_size}}, TensorOptions().with_req_grad(true));
        
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
    
    // INTENTIONALLY BROKEN VERSION: Ignores return values
    void forward_broken(Value x) {
        // WARNING: These should trigger [[nodiscard]] warnings!
        matmul(x, W1);           // Ignored return value
        sigmoid(matmul(x, W1));  // Ignored return value
        matmul(x, W1) + b1;      // Ignored return value (operator+ returns Value)
        
        // More complex ignored operations
        auto h1 = sigmoid(matmul(x, W1) + b1);
        matmul(h1, W2);          // Ignored return value
        h1 + b2;                 // Ignored return value
        
        // Arithmetic operations that should warn
        h1 * h1;                 // Ignored return value
        h1 - h1;                 // Ignored return value
        h1 / h1;                 // Ignored return value
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "NODISCARD Test: Checking Compiler Warnings" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Configuration
    const int input_size = 2;
    const int hidden_size = 2;
    const int output_size = 2;
    const int batch_size = 1;
    
    std::cout << "This test intentionally ignores Value return values." << std::endl;
    std::cout << "If AG_NODISCARD is working, you should see compiler warnings!" << std::endl;
    std::cout << "Look for warnings like: 'ignoring return value of type 'ag::Value''\n" << std::endl;
    
    // Initialize model
    AGMLP model(input_size, hidden_size, output_size);
    
    // Create input
    Tensor input_t = Tensor::zeros(Shape{{batch_size, input_size}}, TensorOptions().with_req_grad(false));
    Value input = make_tensor(input_t, "input");
    
    // Call the broken forward function that ignores return values
    std::cout << "Calling forward_broken() which ignores return values..." << std::endl;
    model.forward_broken(input);
    
    // More standalone tests for nodiscard
    std::cout << "\nAdditional nodiscard tests:" << std::endl;
    
    Value a = make_tensor(Tensor::zeros(Shape{{2, 2}}, TensorOptions()), "a");
    Value b = make_tensor(Tensor::zeros(Shape{{2, 2}}, TensorOptions()), "b");
    
    // These should all trigger warnings:
    a + b;           // WARNING: ignored return value
    a - b;           // WARNING: ignored return value
    a * b;           // WARNING: ignored return value
    a / b;           // WARNING: ignored return value
    sum(a);          // WARNING: ignored return value
    sigmoid(a);      // WARNING: ignored return value
    matmul(a, b);    // WARNING: ignored return value
    
    // This is correct usage (no warning expected):
    Value c = a + b;
    std::cout << "Correct usage: Value c = a + b (no warning expected)" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test complete!" << std::endl;
    std::cout << "Check compilation output for [[nodiscard]] warnings." << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
