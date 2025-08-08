#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <fstream>
#include <memory>

// Layer identification
#define LAYER_NAME "VK_LAYER_frame_interpolation"
#define LAYER_DESCRIPTION "Frame interpolation layer with swapchain monitoring and HUD"

// Layer dispatch table structures
struct LayerInstanceDispatchTable {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkCreateDevice CreateDevice;
};

struct LayerDeviceDispatchTable {
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
    PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
    PFN_vkQueuePresentKHR QueuePresentKHR;
};

// Forward declarations
struct InstanceData;
struct DeviceData;
struct SwapchainData;

// Frame timing data structure
struct FrameTimingData {
    std::chrono::high_resolution_clock::time_point timestamp;
    uint32_t imageIndex;
    VkPresentModeKHR presentMode;
    double frametime_ms;
    uint64_t frameNumber;
};

// HUD overlay state
struct HUDState {
    bool enabled = true;
    std::vector<float> frametimes; // Rolling buffer of frame times
    size_t maxSamples = 120; // Keep 2 seconds at 60fps
    float currentFrametime = 0.0f;
    VkPresentModeKHR currentPresentMode = VK_PRESENT_MODE_FIFO_KHR;
};

// Swapchain tracking data
struct SwapchainData {
    VkSwapchainKHR swapchain;
    VkDevice device;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;
    VkExtent2D extent;
    VkFormat format;
    
    // Frame timing tracking
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    uint64_t frameNumber = 0;
    std::vector<FrameTimingData> frameHistory;
    
    // CSV logging
    std::unique_ptr<std::ofstream> csvFile;
    
    // HUD state
    HUDState hud;
};

// Instance data structure
struct InstanceData {
    VkInstance instance;
    LayerInstanceDispatchTable dispatch;
    std::unordered_map<VkDevice, DeviceData*> devices;
};

// Device data structure  
struct DeviceData {
    VkDevice device;
    LayerDeviceDispatchTable dispatch;
    InstanceData* instance_data;
    std::unordered_map<VkSwapchainKHR, std::unique_ptr<SwapchainData>> swapchains;
};

// Global data
extern std::unordered_map<void*, InstanceData*> instance_map;
extern std::unordered_map<void*, DeviceData*> device_map;
extern std::mutex global_mutex;

// Utility functions
InstanceData* GetInstanceData(VkInstance instance);
DeviceData* GetDeviceData(VkDevice device);
SwapchainData* GetSwapchainData(VkDevice device, VkSwapchainKHR swapchain);
void LogFrameTiming(SwapchainData* swapchain_data, uint32_t imageIndex);
void UpdateHUD(SwapchainData* swapchain_data, double frametime_ms);
void WriteCSVHeader(std::ofstream& file);

// Layer entry points
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName);

// Hooked Vulkan functions
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
VKAPI_ATTR void VKAPI_CALL layer_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
VKAPI_ATTR void VKAPI_CALL layer_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);

// Swapchain interception functions (Stage 0 focus)
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
VKAPI_ATTR void VKAPI_CALL layer_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR VkResult VKAPI_CALL layer_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
VKAPI_ATTR VkResult VKAPI_CALL layer_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
