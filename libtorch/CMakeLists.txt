cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(test_mlp_lib)

set(CMAKE_CUDA_ARCHITECTURES "80")

find_library(NVTOOLSEXT_LIBRARY NAMES nvToolsExt PATH_SUFFIXES lib lib64)
if (NVTOOLSEXT_LIBRARY)
    message(STATUS "Found nvToolsExt library: ${NVTOOLSEXT_LIBRARY}")
    add_library(CUDA::nvToolsExt SHARED IMPORTED)
    set_target_properties(CUDA::nvToolsExt PROPERTIES IMPORTED_LOCATION ${NVTOOLSEXT_LIBRARY})
else()
    # If the library is genuinely not found, you can decide to stop the configuration or provide a warning
    message(WARNING "Could not find nvToolsExt library, build may fail.")
endif()


find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_executable(test_mlp_lib test_mlp_lib.cpp)
target_link_libraries(test_mlp_lib "${TORCH_LIBRARIES}")
set_property(TARGET test_mlp_lib PROPERTY CXX_STANDARD 17) 