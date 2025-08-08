#!/bin/bash

# Vulkan Logger Layer Build and Install Script

set -e

echo "Building Vulkan Logger Layer..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
make -j$(nproc)

echo "Build completed successfully!"

# Install (requires sudo)
echo "Installing layer (requires sudo)..."
sudo make install

# Create user layer directory if it doesn't exist
USER_LAYER_DIR="$HOME/.local/share/vulkan/explicit_layer.d"
mkdir -p "$USER_LAYER_DIR"

# Copy manifest to user directory for easy testing
sudo cp manifests/VK_LAYER_logger.json "$USER_LAYER_DIR/"
sudo chown $USER:$USER "$USER_LAYER_DIR/VK_LAYER_logger.json"

echo "Layer installed successfully!"
echo ""
echo "To test the layer:"
echo "1. Enable the layer: export VK_INSTANCE_LAYERS=VK_LAYER_logger"
echo "2. Run vkcube: vkcube"
echo "3. You should see logging output in the terminal"
echo ""
echo "To test with our simple test program:"
echo "cd build && ./layer_test"
