// =====================
// file: cgadimpl/src/graph.cpp
// =====================
#include "ad/core/graph.hpp"
#include <unordered_set>
#include <functional>
#include <cassert>
#include <sstream>
#include <iostream> // Added for printing


namespace ag {

// --- Node Implementation ---
// Node::Node() = default; 
Node::Node(const Tensor& v, Op op_, bool req_grad, const char* nm) 
    : op(op_), 
      value(v),
      requires_grad_flag_(req_grad),
      debug_name(nm),
      is_leaf(op_ == Op::Leaf)  // Phase 1.1: Mark leaf nodes
{
    // Phase 1.3: Capture execution context
    creation_context.stream = current_stream();
    creation_context.device = v.device();
    
    if (requires_grad_flag_) {
        // CORRECT WAY:
        // 1. Create a TensorOptions object with the correct properties.
        // TensorOptions opts = TensorOptions()
        //                         .with_dtype(v.dtype())
        //                         .with_device(v.device());
        
        // 2. Call the 'zeros' factory with the correct signature (shape, opts).
        grad = OwnTensor::Tensor::zeros(v.shape(), ag::options(v));
    }/*else {
        // If no grad is required, grad can be an empty tensor.
        // grad = Tensor(Shape{}, TensorOptions().with_dtype(v.dtype()).with_device(v.device()));
    }*/
}

// --- Value Implementation ---
// ADDED: Implement the Value helper functions
Tensor& Value::val() { return node->value; }
const Tensor& Value::val() const { return node->value; }
Tensor& Value::grad() { return node->grad; }
const Tensor& Value::grad() const { return node->grad; }
Value::Value() = default;
Value::Value(std::shared_ptr<Node> n) : node(std::move(n)) {}

// NEW: Implementation for the real shape()
const std::vector<int64_t>& Value::shape() const {
    return node->value.shape().dims;
}
// 2d helper
std::pair<int, int> Value::shape_2d() const {
    const auto& dims = node->value.shape().dims;
    if (dims.size() == 0) return {0, 0};
    if (dims.size() == 1) return {1, static_cast<int>(dims[0])};
    // For 2D or more, return the first two dimensions.
    return {static_cast<int>(dims[0]), static_cast<int>(dims[1])};
}

// // --- Factory Implementation ---
// Value make_tensor(const Tensor& v, const char* name) {
//     return Value(std::make_shared<Node> (v, Op::Leaf, v.requires_grad(), name));
// }

// --- Internal implementation for graph traversal ---
static std::vector<Node*> build_topo_order_impl(Node* root) {
    std::vector<Node*> order; order.reserve(256);
    std::unordered_set<Node*> vis; vis.reserve(256);
    std::function<void(Node*)> dfs = [&](Node* n){ if(!n || vis.count(n)) return; vis.insert(n); for(auto& p : n->inputs) dfs(p.get()); order.push_back(n); };
    dfs(root);
    return order; // parents before child
}

// --- Graph Traversal ---
std::vector<Node*> topo_from(Node* root){
    return build_topo_order_impl(root);
}


} 








// static std::pmr::vector<Node*> build_topo_order_impl(Node* root, std::pmr::memory_resource* resource) {
//   // Reserve memory from the Arena (fast bump allocation)
//   std::pmr::vector<Node*> order(resource);
//   order.reserve(256);
 
//   // The visited set does MANY small allocations.
//   // Doing this in an Arena is significantly faster than new/malloc.
//   std::pmr::unordered_set<Node*> vis(resource);
//   vis.reserve(256);


//   std::function<void(Node*)> dfs = [&](Node* n){
//       if(!n || vis.count(n)) return;
//       vis.insert(n); // Allocates node from Arena
//       for(auto& p : n->inputs) dfs(p.get());
//       order.push_back(n); // Allocates array resize from Arena
//   };
 
//   dfs(root);
//   return order;
// }


// // A cache for memoizing topological sorts of graphs.
// static std::unordered_map<Node*, std::vector<Node*>> topo_cache;


// // --- OPTIMIZED CACHING VERSION ---
// // Uses a temporary Arena as a "Scratchpad" for the calculation.
// std::vector<Node*> topo_from(Node* root) {
//   auto it = topo_cache.find(root);
//   if (it != topo_cache.end()) {
//       return it->second;
//   }

//   std::cout << "--- Building and Caching (Using Scratchpad Arena) ---" << std::endl;

//   // Create a temporary Arena on the stack.
//   // give it a moderate size buffer
//   // This is incredibly fast because it's just a stack variable.
//   ad::memory::Arena scratch_arena(16 * 1024);

//   // 2. Build the order using the Arena.
//   // 'vis' and 'pmr_order' will allocate strictly from 'scratch_arena'.
//   // No calls to global new/delete happen here.
//   auto pmr_order = build_topo_order_impl(root, &scratch_arena);

//   // 3. Copy results to the permanent cache (Standard Heap Vector).
//   topo_cache[root] = std::vector<Node*>(pmr_order.begin(), pmr_order.end());

//   // 4. 'scratch_arena' goes out of scope here.
//   // It instantly frees all memory used by 'vis' and 'pmr_order' by simply
//   // resetting a pointer. No need to walk the list or free nodes individually.
 
//   return topo_cache[root]; 
// }

// // --- EXPLICIT ARENA VERSION ---
// // Returns a PMR vector strictly tied to the provided arena.
// std::pmr::vector<Node*> topo_from(Node* root, ad::memory::Arena& arena) {
//   // We pass the address of your specific Arena type
//   return build_topo_order_impl(root, &arena);
// } 


// } // namespace ag