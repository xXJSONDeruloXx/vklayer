#!/bin/bash

# Vulkan Layer Test Suite
# Tests all three layers: logger, green_tint, and text_overlay

set -e

echo "==============================================="
echo "         Vulkan Layer Test Suite"
echo "==============================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to test a layer
test_layer() {
    local layer_name=$1
    local description=$2
    
    echo -e "${BLUE}Testing $layer_name:${NC} $description"
    echo "Command: export VK_INSTANCE_LAYERS=$layer_name && vkcube --c 60"
    echo ""
    
    export VK_INSTANCE_LAYERS=$layer_name
    timeout 5s vkcube --c 60 || true
    unset VK_INSTANCE_LAYERS
    
    echo ""
    echo -e "${GREEN}✓ $layer_name test completed${NC}"
    echo ""
}

# Build if needed
if [ ! -f "build/lib/VK_LAYER_logger.so" ] || \
   [ ! -f "build/lib/VK_LAYER_green_tint.so" ] || \
   [ ! -f "build/lib/VK_LAYER_text_overlay.so" ]; then
    echo -e "${YELLOW}Building layers...${NC}"
    ./build.sh
    echo ""
fi

# Test 1: Baseline (no layers)
echo -e "${BLUE}Baseline Test:${NC} vkcube without layers"
echo "Command: vkcube --c 60"
echo ""
timeout 5s vkcube --c 60 || true
echo ""
echo -e "${GREEN}✓ Baseline test completed${NC}"
echo ""

# Test 2: Logger layer
test_layer "VK_LAYER_logger" "Logs Vulkan API calls"

# Test 3: Green tint layer  
test_layer "VK_LAYER_green_tint" "Applies green tint effect"

# Test 4: Text overlay layer
test_layer "VK_LAYER_text_overlay" "Overlays Lorem Ipsum text effects"

# Test 5: Multiple layers
echo -e "${BLUE}Multiple Layers Test:${NC} Logger + Text Overlay"
echo "Command: export VK_INSTANCE_LAYERS=VK_LAYER_logger:VK_LAYER_text_overlay && vkcube --c 60"
echo ""
export VK_INSTANCE_LAYERS=VK_LAYER_logger:VK_LAYER_text_overlay
timeout 5s vkcube --c 60 || true
unset VK_INSTANCE_LAYERS
echo ""
echo -e "${GREEN}✓ Multiple layers test completed${NC}"
echo ""

echo "==============================================="
echo -e "${GREEN}         All Tests Completed!${NC}"
echo "==============================================="
echo ""
echo "Available layers:"
echo "  • VK_LAYER_logger      - API call logging"
echo "  • VK_LAYER_green_tint  - Green tint effect"  
echo "  • VK_LAYER_text_overlay - Lorem Ipsum text overlay"
echo ""
echo "Usage:"
echo "  export VK_INSTANCE_LAYERS=VK_LAYER_logger"
echo "  vkcube"
echo ""
