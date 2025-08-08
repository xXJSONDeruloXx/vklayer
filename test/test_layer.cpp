#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <cstring>

int main() {
    std::cout << "Testing Vulkan Logger Layer..." << std::endl;
    
    // Create application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Layer Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    // Create instance info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Create instance
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance! Result: " << result << std::endl;
        return -1;
    }
    
    std::cout << "Vulkan instance created successfully!" << std::endl;
    
    // Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        std::cout << "No physical devices found!" << std::endl;
    } else {
        std::cout << "Found " << deviceCount << " physical device(s)" << std::endl;
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        // Get properties of first device
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[0], &deviceProperties);
        std::cout << "Device name: " << deviceProperties.deviceName << std::endl;
    }
    
    // Clean up
    vkDestroyInstance(instance, nullptr);
    std::cout << "Test completed!" << std::endl;
    
    return 0;
}
