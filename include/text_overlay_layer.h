#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <map>
#include <mutex>
#include <cstring>
#include <iostream>
#include <chrono>

#define LAYER_NAME "VK_LAYER_text_overlay"

// Forward declarations
struct InstanceData;
struct DeviceData;

// Simple dispatch table structures
struct LayerInstanceDispatchTable {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
};

struct LayerDeviceDispatchTable {
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
    PFN_vkCmdEndRenderPass CmdEndRenderPass;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdDrawIndexed CmdDrawIndexed;
    PFN_vkCmdSetViewport CmdSetViewport;
    PFN_vkCmdSetScissor CmdSetScissor;
    PFN_vkQueuePresentKHR QueuePresentKHR;
};

// Global state
extern std::map<VkInstance, InstanceData*> instance_map;
extern std::map<VkDevice, DeviceData*> device_map;
extern std::mutex global_mutex;

// Instance data structure
struct InstanceData {
    LayerInstanceDispatchTable vtable;
    VkInstance instance;
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
};

// Device data structure
struct DeviceData {
    LayerDeviceDispatchTable vtable;
    VkDevice device;
    VkPhysicalDevice physical_device;
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    
    // Text overlay resources
    VkBuffer text_buffer;
    VkDeviceMemory text_buffer_memory;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline text_pipeline;
    bool text_overlay_initialized;
};

// Helper functions
InstanceData* GetInstanceData(VkInstance instance);
DeviceData* GetDeviceData(VkDevice device);
void LogAPICall(const char* function_name, const char* message = nullptr);

// Vulkan Layer Functions
extern "C" {
    // Instance functions
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance);

    VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator);

    VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
        VkInstance instance,
        uint32_t* pPhysicalDeviceCount,
        VkPhysicalDevice* pPhysicalDevices);

    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties);

    // Device functions
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice);

    VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator);

    // Command buffer functions for text overlay
    VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
        VkCommandBuffer commandBuffer,
        const VkRenderPassBeginInfo* pRenderPassBegin,
        VkSubpassContents contents);

    VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
        VkCommandBuffer commandBuffer);

    VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
        VkCommandBuffer commandBuffer,
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance);

    VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo);

    // Proc addr functions
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
        VkInstance instance,
        const char* pName);

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
        VkDevice device,
        const char* pName);

    // Layer info functions
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
