cmake_minimum_required(VERSION 3.16)
project(VulkanLayers VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Create the logger layer library
add_library(VK_LAYER_logger SHARED
    src/logger_layer.cpp
)

target_include_directories(VK_LAYER_logger PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    include
)

target_link_libraries(VK_LAYER_logger PRIVATE
    ${Vulkan_LIBRARIES}
    dl
)

# Create the green tint layer library
add_library(VK_LAYER_green_tint SHARED
    src/green_tint_layer.cpp
)

target_include_directories(VK_LAYER_green_tint PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    include
)

target_link_libraries(VK_LAYER_green_tint PRIVATE
    ${Vulkan_LIBRARIES}
    dl
)

# Create the text overlay layer library
add_library(VK_LAYER_text_overlay SHARED
    src/text_overlay_layer.cpp
)

target_include_directories(VK_LAYER_text_overlay PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    include
)

target_link_libraries(VK_LAYER_text_overlay PRIVATE
    ${Vulkan_LIBRARIES}
    dl
)

# Set library properties for all layers
set_target_properties(VK_LAYER_logger VK_LAYER_green_tint VK_LAYER_text_overlay PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    PREFIX ""
)

# Install layer manifests
configure_file(
    ${CMAKE_SOURCE_DIR}/manifests/VK_LAYER_logger.json.in
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_logger.json
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/manifests/VK_LAYER_green_tint.json.in
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_green_tint.json
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/manifests/VK_LAYER_text_overlay.json.in
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_text_overlay.json
    @ONLY
)

# Installation
install(TARGETS VK_LAYER_logger VK_LAYER_green_tint VK_LAYER_text_overlay
    LIBRARY DESTINATION lib
)

install(FILES 
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_logger.json
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_green_tint.json
    ${CMAKE_BINARY_DIR}/manifests/VK_LAYER_text_overlay.json
    DESTINATION share/vulkan/explicit_layer.d
)

# Create test executable
add_executable(layer_test
    test/test_layer.cpp
)

target_include_directories(layer_test PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(layer_test PRIVATE
    ${Vulkan_LIBRARIES}
)
