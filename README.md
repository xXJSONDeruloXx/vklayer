# Vulkan Layers Collection

A collection of educational Vulkan validation layers for Linux, demonstrating layer development techniques and providing useful debugging and visual effects capabilities.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build](https://img.shields.io/badge/Build-CMake-blue.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.linux.org/)

## 🎯 Overview

This project contains three Vulkan layers designed for education, debugging, and visual effects:

- **🔍 VK_LAYER_logger** - Comprehensive API call logging with timestamps
- **🎨 VK_LAYER_green_tint** - Visual effect layer that applies green tinting
- **📝 VK_LAYER_text_overlay** - Text overlay system demonstrating Lorem Ipsum content

## 🚀 Features

### Logger Layer
- Real-time Vulkan API call logging
- Timestamped output with millisecond precision
- Instance and device-level function interception
- Thread-safe operation

### Green Tint Layer  
- Subtle green color overlay effect
- Render pass and present queue interception
- Shader pipeline integration
- Non-intrusive visual modification

### Text Overlay Layer
- Lorem Ipsum text content overlay
- Viewport and scissor rectangle modification
- Bitmap font framework (extensible)
- Background tinting and text area effects

## 📋 Prerequisites

- **Operating System**: Linux (tested on Fedora/RHEL-based systems)
- **Vulkan SDK**: 1.3.0 or later
- **CMake**: 3.16 or later
- **Compiler**: GCC/Clang with C++17 support
- **Dependencies**: 
  - `vulkan-devel` (development headers)
  - `vulkan-tools` (for vkcube testing)

### Installation on Fedora/RHEL:
```bash
sudo dnf install vulkan-devel vulkan-tools cmake gcc-c++
```

### Installation on Ubuntu/Debian:
```bash
sudo apt install libvulkan-dev vulkan-tools cmake build-essential
```

## 🛠️ Building

1. **Clone the repository:**
   ```bash
   git clone https://github.com/xXJSONDeruloXx/vklayer.git
   cd vklayer
   ```

2. **Build all layers:**
   ```bash
   ./build.sh
   ```

   This will:
   - Configure CMake build system
   - Compile all three layers
   - Install to system-wide Vulkan layer directory
   - Generate proper manifest files

## 🧪 Testing

### Quick Test Suite
Run the comprehensive test suite:
```bash
./test_layers.sh
```

This will test all layers individually and in combination.

### Manual Testing

**Test individual layers:**
```bash
# Logger layer
export VK_INSTANCE_LAYERS=VK_LAYER_logger
vkcube

# Green tint layer
export VK_INSTANCE_LAYERS=VK_LAYER_green_tint
vkcube

# Text overlay layer
export VK_INSTANCE_LAYERS=VK_LAYER_text_overlay
vkcube
```

**Test multiple layers:**
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_logger:VK_LAYER_text_overlay
vkcube
```

**Clean up environment:**
```bash
unset VK_INSTANCE_LAYERS
```

## 📁 Project Structure

```
vklayer/
├── CMakeLists.txt           # Build configuration
├── build.sh                 # Build script
├── test_layers.sh          # Test suite
├── README.md               # This file
├── .gitignore              # Git ignore rules
│
├── include/                # Header files
│   ├── logger_layer.h
│   └── text_overlay_layer.h
│
├── src/                    # Source files
│   ├── logger_layer.cpp
│   ├── green_tint_layer.cpp
│   └── text_overlay_layer.cpp
│
├── manifests/              # Layer manifest templates
│   ├── VK_LAYER_logger.json.in
│   ├── VK_LAYER_green_tint.json.in
│   └── VK_LAYER_text_overlay.json.in
│
└── test/                   # Test programs
    └── test_layer.cpp
```

## 🔧 Development

### Layer Architecture
Each layer follows the standard Vulkan layer pattern:
- **Chain interception**: Properly chains with other layers
- **Dispatch tables**: Maintains instance and device dispatch tables
- **Thread safety**: Uses mutexes for concurrent access
- **Error handling**: Proper Vulkan error code propagation

### Adding New Layers
1. Create header file in `include/`
2. Implement source file in `src/`
3. Add manifest template in `manifests/`
4. Update `CMakeLists.txt`
5. Update test scripts

### Code Style
- C++17 standard
- Vulkan best practices
- Thread-safe implementations
- Comprehensive error handling

## 🐛 Troubleshooting

### Layer Not Loading
1. Check that libraries are installed:
   ```bash
   ls /usr/local/lib/VK_LAYER_*.so
   ```

2. Verify manifest files:
   ```bash
   ls /usr/local/share/vulkan/explicit_layer.d/VK_LAYER_*.json
   ```

3. Check Vulkan installation:
   ```bash
   vulkaninfo
   ```

### Build Issues
- Ensure Vulkan SDK is properly installed
- Check CMake version compatibility
- Verify compiler supports C++17

### Runtime Issues
- Check console output for layer messages
- Verify `VK_INSTANCE_LAYERS` environment variable
- Test with simple Vulkan applications first

## 📚 Educational Value

This project demonstrates:
- **Vulkan Layer Development**: Complete layer implementation patterns
- **API Interception**: Function pointer chaining and dispatch tables
- **Thread Safety**: Concurrent access patterns in graphics APIs
- **Build Systems**: CMake configuration for Vulkan projects
- **Testing**: Automated testing strategies for graphics layers

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Vulkan specification and layer development guidelines
- Khronos Group for Vulkan API
- Community examples and best practices

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/xXJSONDeruloXx/vklayer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/xXJSONDeruloXx/vklayer/discussions)

---

**Note**: This is an educational project designed to demonstrate Vulkan layer development techniques. Use in production environments should be carefully evaluated.
