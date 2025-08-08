#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

// Layer name and description
#define LAYER_NAME "VK_LAYER_logger"
#define LAYER_DESCRIPTION "Simple Vulkan API call logger"

// Simple dispatch table structures
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
};

// Function pointer types for dispatch tables
struct InstanceData {
    LayerInstanceDispatchTable vtable;
    VkInstance instance;
    std::ofstream* log_file;
};

struct DeviceData {
    LayerDeviceDispatchTable vtable;
    VkDevice device;
    InstanceData* instance_data;
};

// Global data
extern std::unordered_map<void*, InstanceData*> instance_map;
extern std::unordered_map<void*, DeviceData*> device_map;
extern std::mutex global_mutex;

// Utility functions
InstanceData* GetInstanceData(VkInstance instance);
DeviceData* GetDeviceData(VkDevice device);
void LogAPICall(const std::string& function_name, const std::string& details = "");
std::string GetCurrentTimestamp();

// Layer entry points
extern "C" {
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance);

    VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator);

    VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice);

    VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
        VkInstance instance,
        uint32_t* pPhysicalDeviceCount,
        VkPhysicalDevice* pPhysicalDevices);

    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties);

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
        VkInstance instance,
        const char* pName);

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
        VkDevice device,
        const char* pName);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t* pPropertyCount,
        VkLayerProperties* pProperties);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pPropertyCount,
        VkLayerProperties* pProperties);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties);
}
