# include <torch/torch.h>
# include <iostream>

// Define the 3-layer MLP class
struct SimpleMLP : torch::nn::Module {
    SimpleMLP() {
        // First layer: input (2) -> hidden (2)
        // Bias is set to False as the user only mentioned weight matrices
        fc1 = register_module("fc1", torch::nn::Linear(2, 2));
        // Second layer: hidden (2) -> hidden (2)
        fc2 = register_module("fc2", torch::nn::Linear(2, 2));
        // Third layer: hidden (2) -> output (2)
        fc3 = register_module("fc3", torch::nn::Linear(2, 2)); 

        // Initialize weight matrices with random 2x2 values
        // LibTorch nn::Linear initializes weights by default, but we'll explicitly set them as requested
        fc1->weight.data().copy_(torch::rand({2, 2}));
        fc2->weight.data().copy_(torch::rand({2, 2}));
        fc3->weight.data().copy_(torch::rand({2, 2}));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::sigmoid(fc1->forward(x));
        x = torch::sigmoid(fc2->forward(x));
        x = fc3->forward(x); // No activation on the output layer for a general MLP
        return x;
    }

    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr};
};

int main() {
    // Instantiate the model
    auto model = std::make_shared<SimpleMLP>();

    std::cout << "MLP Model Structure (with Sigmoid):" << std::endl;
    std::cout << model << std::endl;

    std::cout << "\nInitial Random Weight Matrices (after Sigmoid change and re-instantiation):" << std::endl;
    std::cout << "Layer 1 Weights:\n" << model->fc1->weight.data() << std::endl;
    std::cout << "Layer 2 Weights:\n" << model->fc2->weight.data() << std::endl;
    std::cout << "Layer 3 Weights:\n" << model->fc3->weight.data() << std::endl;

    // Create a dummy input tensor (batch_size=1, input_features=2)
    torch::Tensor dummy_input = torch::randn({1, 2});
    std::cout << "\nDummy Input:\n" << dummy_input << std::endl;

    // Create a dummy target for training
    torch::Tensor dummy_target = torch::randn({1, 2});
    std::cout << "Dummy Target:\n" << dummy_target << std::endl;

    // Perform an initial forward pass to show output before training
    torch::Tensor output_before_training = model->forward(dummy_input);
    std::cout << "\nOutput of the MLP before training:\n" << output_before_training << std::endl;

    // Define Loss function and Optimizer
    torch::nn::MSELoss criterion;
    torch::optim::SGD optimizer(model->parameters(), /*lr=*/0.1);

    // Training loop
    int num_iterations = 100;
    std::cout << "\nTraining for " << num_iterations << " iterations..." << std::endl;

    for (int i = 0; i < num_iterations; ++i) {
        // Zero the gradients
        optimizer.zero_grad();

        // Forward pass
        torch::Tensor output = model->forward(dummy_input);

        // Calculate loss
        torch::Tensor loss = criterion(output, dummy_target);

        // Backward pass and optimize
        loss.backward();
        optimizer.step();

        if ((i + 1) % 10 == 0) {
            std::cout << "Iteration [" << (i + 1) << "/" << num_iterations << "], Loss: " << loss.item<double>() << std::endl;
        }
    }

    std::cout << "\nTraining complete." << std::endl;

    // Show output after training
    torch::Tensor output_after_training = model->forward(dummy_input);
    std::cout << "Output of the MLP after training:\n" << output_after_training << std::endl;
    std::cout << "Final Loss:\n" << criterion(output_after_training, dummy_target).item<double>() << std::endl;

    std::cout << "\nFinal Weight Matrices after Training:" << std::endl;
    std::cout << "Layer 1 Weights:\n" << model->fc1->weight.data() << std::endl;
    std::cout << "Layer 2 Weights:\n" << model->fc2->weight.data() << std::endl;
    std::cout << "Layer 3 Weights:\n" << model->fc3->weight.data() << std::endl;

    return 0;
}