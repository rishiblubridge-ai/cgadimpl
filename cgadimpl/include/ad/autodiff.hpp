// =====================
// file: include/ag/autodiff.hpp (declarations only)
// =====================
#pragma once
#include <unordered_map>
#include "ad/ops.hpp"
#include "ad/nodiscard.hpp" 
//#include "ad/tensor.hpp"  


namespace ag {


void zero_grad(const Value& root);
void backward (const Value& root, const Tensor* grad_seed=nullptr);

AG_NODISCARD Tensor jvp (const Value& root, const std::unordered_map<Node*, Tensor>& seed);


} // namespace ag