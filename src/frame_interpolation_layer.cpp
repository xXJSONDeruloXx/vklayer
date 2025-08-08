#include "frame_interpolation_layer.h"
#include <cstring>
#include <sstream>
#include <iomanip>

// Global data
std::unordered_map<void*, InstanceData*> instance_map;
std::unordered_map<void*, DeviceData*> device_map;
std::mutex global_mutex;

// Layer properties
static const VkLayerProperties layer_props = {
    LAYER_NAME,
    VK_MAKE_VERSION(1, 0, 0),
    1,
    LAYER_DESCRIPTION,
};

// Utility functions
InstanceData* GetInstanceData(VkInstance instance) {
    std::lock_guard<std::mutex> lock(global_mutex);
    auto it = instance_map.find(instance);
    return (it != instance_map.end()) ? it->second : nullptr;
}

DeviceData* GetDeviceData(VkDevice device) {
    std::lock_guard<std::mutex> lock(global_mutex);
    auto it = device_map.find(device);
    return (it != device_map.end()) ? it->second : nullptr;
}

SwapchainData* GetSwapchainData(VkDevice device, VkSwapchainKHR swapchain) {
    DeviceData* device_data = GetDeviceData(device);
    if (!device_data) return nullptr;
    
    auto it = device_data->swapchains.find(swapchain);
    return (it != device_data->swapchains.end()) ? it->second.get() : nullptr;
}

void LogFrameTiming(SwapchainData* swapchain_data, uint32_t imageIndex) {
    auto now = std::chrono::high_resolution_clock::now();
    
    if (swapchain_data->frameNumber > 0) {
        auto frametime = std::chrono::duration<double, std::milli>(
            now - swapchain_data->lastFrameTime).count();
        
        FrameTimingData timing_data;
        timing_data.timestamp = now;
        timing_data.imageIndex = imageIndex;
        timing_data.presentMode = swapchain_data->presentMode;
        timing_data.frametime_ms = frametime;
        timing_data.frameNumber = swapchain_data->frameNumber;
        
        swapchain_data->frameHistory.push_back(timing_data);
        
        // Keep only last 1000 frames
        if (swapchain_data->frameHistory.size() > 1000) {
            swapchain_data->frameHistory.erase(swapchain_data->frameHistory.begin());
        }
        
        // Update HUD
        UpdateHUD(swapchain_data, frametime);
        
        // Log to CSV
        if (swapchain_data->csvFile && swapchain_data->csvFile->is_open()) {
            *swapchain_data->csvFile << timing_data.frameNumber << ","
                                    << timing_data.frametime_ms << ","
                                    << timing_data.imageIndex << ","
                                    << timing_data.presentMode << std::endl;
        }
        
        // Console logging every 60 frames
        if (swapchain_data->frameNumber % 60 == 0) {
            std::cout << "[FRAME_INTERP] Frame " << swapchain_data->frameNumber 
                     << ": " << std::fixed << std::setprecision(2) << frametime << "ms"
                     << " (FPS: " << (1000.0 / frametime) << ")"
                     << " Present Mode: " << swapchain_data->presentMode
                     << " Image Index: " << imageIndex << std::endl;
        }
    }
    
    swapchain_data->lastFrameTime = now;
    swapchain_data->frameNumber++;
}

void UpdateHUD(SwapchainData* swapchain_data, double frametime_ms) {
    if (!swapchain_data->hud.enabled) return;
    
    swapchain_data->hud.currentFrametime = frametime_ms;
    swapchain_data->hud.frametimes.push_back(frametime_ms);
    
    // Keep rolling buffer of frame times
    if (swapchain_data->hud.frametimes.size() > swapchain_data->hud.maxSamples) {
        swapchain_data->hud.frametimes.erase(swapchain_data->hud.frametimes.begin());
    }
}

void WriteCSVHeader(std::ofstream& file) {
    file << "FrameNumber,FrametimeMs,ImageIndex,PresentMode" << std::endl;
}

// Hooked Vulkan functions
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    VkLayerInstanceCreateInfo* chain_info = 
        const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext));
    
    while (chain_info && 
           (chain_info->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
            chain_info->function != VK_LAYER_LINK_INFO)) {
        chain_info = const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(chain_info->pNext));
    }
    
    if (chain_info == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = 
        reinterpret_cast<PFN_vkCreateInstance>(fpGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    
    if (fpCreateInstance == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) return result;
    
    InstanceData* instance_data = new InstanceData();
    instance_data->instance = *pInstance;
    instance_data->dispatch.GetInstanceProcAddr = fpGetInstanceProcAddr;
    instance_data->dispatch.DestroyInstance = 
        reinterpret_cast<PFN_vkDestroyInstance>(fpGetInstanceProcAddr(*pInstance, "vkDestroyInstance"));
    instance_data->dispatch.EnumeratePhysicalDevices = 
        reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(fpGetInstanceProcAddr(*pInstance, "vkEnumeratePhysicalDevices"));
    instance_data->dispatch.GetPhysicalDeviceProperties = 
        reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(fpGetInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceProperties"));
    instance_data->dispatch.CreateDevice = 
        reinterpret_cast<PFN_vkCreateDevice>(fpGetInstanceProcAddr(*pInstance, "vkCreateDevice"));
    
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        instance_map[*pInstance] = instance_data;
    }
    
    std::cout << "[FRAME_INTERP] Layer initialized for instance " << *pInstance << std::endl;
    return result;
}

VKAPI_ATTR void VKAPI_CALL layer_vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    InstanceData* instance_data = GetInstanceData(instance);
    if (instance_data) {
        instance_data->dispatch.DestroyInstance(instance, pAllocator);
        
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            instance_map.erase(instance);
        }
        delete instance_data;
        std::cout << "[FRAME_INTERP] Instance destroyed" << std::endl;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    VkLayerDeviceCreateInfo* chain_info = 
        const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
    
    while (chain_info && 
           (chain_info->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO ||
            chain_info->function != VK_LAYER_LINK_INFO)) {
        chain_info = const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(chain_info->pNext));
    }
    
    if (chain_info == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = 
        reinterpret_cast<PFN_vkCreateDevice>(fpGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateDevice"));
    
    if (fpCreateDevice == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    
    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) return result;
    
    DeviceData* device_data = new DeviceData();
    device_data->device = *pDevice;
    device_data->dispatch.GetDeviceProcAddr = fpGetDeviceProcAddr;
    device_data->dispatch.DestroyDevice = 
        reinterpret_cast<PFN_vkDestroyDevice>(fpGetDeviceProcAddr(*pDevice, "vkDestroyDevice"));
    device_data->dispatch.CreateSwapchainKHR = 
        reinterpret_cast<PFN_vkCreateSwapchainKHR>(fpGetDeviceProcAddr(*pDevice, "vkCreateSwapchainKHR"));
    device_data->dispatch.DestroySwapchainKHR = 
        reinterpret_cast<PFN_vkDestroySwapchainKHR>(fpGetDeviceProcAddr(*pDevice, "vkDestroySwapchainKHR"));
    device_data->dispatch.AcquireNextImageKHR = 
        reinterpret_cast<PFN_vkAcquireNextImageKHR>(fpGetDeviceProcAddr(*pDevice, "vkAcquireNextImageKHR"));
    device_data->dispatch.QueuePresentKHR = 
        reinterpret_cast<PFN_vkQueuePresentKHR>(fpGetDeviceProcAddr(*pDevice, "vkQueuePresentKHR"));
    
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        device_map[*pDevice] = device_data;
    }
    
    std::cout << "[FRAME_INTERP] Device created" << std::endl;
    return result;
}

VKAPI_ATTR void VKAPI_CALL layer_vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    DeviceData* device_data = GetDeviceData(device);
    if (device_data) {
        device_data->dispatch.DestroyDevice(device, pAllocator);
        
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            device_map.erase(device);
        }
        delete device_data;
        std::cout << "[FRAME_INTERP] Device destroyed" << std::endl;
    }
}

// Stage 0 swapchain interception functions
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain) {
    
    DeviceData* device_data = GetDeviceData(device);
    if (!device_data) return VK_ERROR_INITIALIZATION_FAILED;
    
    VkResult result = device_data->dispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    if (result == VK_SUCCESS) {
        auto swapchain_data = std::make_unique<SwapchainData>();
        swapchain_data->swapchain = *pSwapchain;
        swapchain_data->device = device;
        swapchain_data->presentMode = pCreateInfo->presentMode;
        swapchain_data->imageCount = pCreateInfo->minImageCount;
        swapchain_data->extent = pCreateInfo->imageExtent;
        swapchain_data->format = pCreateInfo->imageFormat;
        swapchain_data->lastFrameTime = std::chrono::high_resolution_clock::now();
        
        // Initialize CSV logging
        std::string filename = "frame_timing_" + std::to_string(reinterpret_cast<uintptr_t>(*pSwapchain)) + ".csv";
        swapchain_data->csvFile = std::make_unique<std::ofstream>(filename);
        if (swapchain_data->csvFile->is_open()) {
            WriteCSVHeader(*swapchain_data->csvFile);
        }
        
        device_data->swapchains[*pSwapchain] = std::move(swapchain_data);
        
        std::cout << "[FRAME_INTERP] Swapchain created: " << pCreateInfo->imageExtent.width 
                 << "x" << pCreateInfo->imageExtent.height
                 << " Present Mode: " << pCreateInfo->presentMode 
                 << " Format: " << pCreateInfo->imageFormat << std::endl;
    }
    
    return result;
}

VKAPI_ATTR void VKAPI_CALL layer_vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator) {
    
    DeviceData* device_data = GetDeviceData(device);
    if (device_data) {
        SwapchainData* swapchain_data = GetSwapchainData(device, swapchain);
        if (swapchain_data && swapchain_data->csvFile) {
            swapchain_data->csvFile->close();
        }
        
        device_data->swapchains.erase(swapchain);
        device_data->dispatch.DestroySwapchainKHR(device, swapchain, pAllocator);
        
        std::cout << "[FRAME_INTERP] Swapchain destroyed" << std::endl;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL layer_vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* pImageIndex) {
    
    DeviceData* device_data = GetDeviceData(device);
    if (!device_data) return VK_ERROR_INITIALIZATION_FAILED;
    
    VkResult result = device_data->dispatch.AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    
    if (result == VK_SUCCESS) {
        SwapchainData* swapchain_data = GetSwapchainData(device, swapchain);
        if (swapchain_data) {
            // Record timing data on acquire (start of frame)
            LogFrameTiming(swapchain_data, *pImageIndex);
        }
    }
    
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL layer_vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    
    // Find device associated with this queue (simplified approach)
    DeviceData* device_data = nullptr;
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        for (auto& pair : device_map) {
            device_data = pair.second;
            break; // For simplicity, assume single device
        }
    }
    
    if (!device_data) return VK_ERROR_INITIALIZATION_FAILED;
    
    VkResult result = device_data->dispatch.QueuePresentKHR(queue, pPresentInfo);
    
    // Log present completion
    if (result == VK_SUCCESS && pPresentInfo->swapchainCount > 0) {
        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            SwapchainData* swapchain_data = GetSwapchainData(device_data->device, pPresentInfo->pSwapchains[i]);
            if (swapchain_data) {
                // Could add present-specific timing here if needed
            }
        }
    }
    
    return result;
}

// Layer entry points
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    // Handle global functions
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr);
    }
    if (strcmp(pName, "vkCreateInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkCreateInstance);
    }
    if (strcmp(pName, "vkDestroyInstance") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkDestroyInstance);
    }
    if (strcmp(pName, "vkCreateDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkCreateDevice);
    }
    
    // Pass through to next layer
    if (instance != VK_NULL_HANDLE) {
        InstanceData* instance_data = GetInstanceData(instance);
        if (instance_data) {
            return instance_data->dispatch.GetInstanceProcAddr(instance, pName);
        }
    }
    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    // Handle device functions
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
    }
    if (strcmp(pName, "vkDestroyDevice") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkDestroyDevice);
    }
    if (strcmp(pName, "vkCreateSwapchainKHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkCreateSwapchainKHR);
    }
    if (strcmp(pName, "vkDestroySwapchainKHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkDestroySwapchainKHR);
    }
    if (strcmp(pName, "vkAcquireNextImageKHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkAcquireNextImageKHR);
    }
    if (strcmp(pName, "vkQueuePresentKHR") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(layer_vkQueuePresentKHR);
    }
    
    // Pass through to next layer
    if (device != VK_NULL_HANDLE) {
        DeviceData* device_data = GetDeviceData(device);
        if (device_data) {
            return device_data->dispatch.GetDeviceProcAddr(device, pName);
        }
    }
    return nullptr;
}

// Required layer entry points
extern "C" {
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t* pCount, VkLayerProperties* pProperties) {
        if (pProperties == nullptr) {
            *pCount = 1;
            return VK_SUCCESS;
        }
        
        if (*pCount < 1) {
            return VK_INCOMPLETE;
        }
        
        memcpy(pProperties, &layer_props, sizeof(layer_props));
        *pCount = 1;
        return VK_SUCCESS;
    }
    
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pCount, VkLayerProperties* pProperties) {
        return vkEnumerateInstanceLayerProperties(pCount, pProperties);
    }
}
