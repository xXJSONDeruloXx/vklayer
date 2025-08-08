#include "logger_layer.h"
#include <cstring>
#include <sstream>

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

std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void LogAPICall(const std::string& function_name, const std::string& details) {
    std::string timestamp = GetCurrentTimestamp();
    std::cout << "[" << timestamp << "] VULKAN_LAYER: " << function_name;
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << std::endl;
    std::cout.flush(); // Force immediate output
}

// Vulkan API implementations
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    LogAPICall("vkCreateInstance", "Creating Vulkan instance");
    
    // Get the layer's instance proc addr
    VkLayerInstanceCreateInfo* chain_info = 
        (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    
    while (chain_info && 
           !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
             chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerInstanceCreateInfo*)chain_info->pNext;
    }
    
    if (!chain_info) {
        LogAPICall("vkCreateInstance", "No chain info - calling next layer directly");
        // If no chain info, we're the last layer - call the driver directly
        PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
        if (!fpCreateInstance) {
            LogAPICall("vkCreateInstance", "ERROR: Cannot get vkCreateInstance");
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
        if (result == VK_SUCCESS) {
            // Create minimal instance data
            InstanceData* instance_data = new InstanceData();
            instance_data->instance = *pInstance;
            // Initialize basic dispatch table
            instance_data->vtable.GetInstanceProcAddr = vkGetInstanceProcAddr;
            instance_data->vtable.DestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(*pInstance, "vkDestroyInstance");
            instance_data->vtable.EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(*pInstance, "vkEnumeratePhysicalDevices");
            instance_data->vtable.GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceProperties");
            instance_data->vtable.CreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(*pInstance, "vkCreateDevice");
            
            std::lock_guard<std::mutex> lock(global_mutex);
            instance_map[*pInstance] = instance_data;
            LogAPICall("vkCreateInstance", "Instance created successfully");
        }
        return result;
    }
    
    PFN_vkGetInstanceProcAddr gpa = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance create_instance = 
        (PFN_vkCreateInstance)gpa(VK_NULL_HANDLE, "vkCreateInstance");
    
    if (!create_instance) {
        LogAPICall("vkCreateInstance", "ERROR: Failed to get next vkCreateInstance");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    
    VkResult result = create_instance(pCreateInfo, pAllocator, pInstance);
    
    if (result == VK_SUCCESS) {
        // Create instance data
        InstanceData* instance_data = new InstanceData();
        instance_data->instance = *pInstance;
        
        // Initialize dispatch table
        LayerInstanceDispatchTable* pTable = &instance_data->vtable;
        
        // Get function pointers
        pTable->GetInstanceProcAddr = gpa;
        pTable->DestroyInstance = (PFN_vkDestroyInstance)gpa(*pInstance, "vkDestroyInstance");
        pTable->CreateDevice = (PFN_vkCreateDevice)gpa(*pInstance, "vkCreateDevice");
        pTable->EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)gpa(*pInstance, "vkEnumeratePhysicalDevices");
        pTable->GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)gpa(*pInstance, "vkGetPhysicalDeviceProperties");
        
        // Store instance data
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            instance_map[*pInstance] = instance_data;
        }
        
        LogAPICall("vkCreateInstance", "Instance created successfully");
    } else {
        LogAPICall("vkCreateInstance", "ERROR: Instance creation failed");
    }
    
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    LogAPICall("vkDestroyInstance", "Destroying Vulkan instance");
    
    InstanceData* instance_data = GetInstanceData(instance);
    if (instance_data && instance_data->vtable.DestroyInstance) {
        instance_data->vtable.DestroyInstance(instance, pAllocator);
    } else {
        // Call driver directly
        PFN_vkDestroyInstance fpDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
        if (fpDestroyInstance) {
            fpDestroyInstance(instance, pAllocator);
        }
    }
    
    if (instance_data) {
        std::lock_guard<std::mutex> lock(global_mutex);
        instance_map.erase(instance);
        delete instance_data;
        LogAPICall("vkDestroyInstance", "Instance destroyed successfully");
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices) {
    
    LogAPICall("vkEnumeratePhysicalDevices", "Enumerating physical devices");
    
    InstanceData* instance_data = GetInstanceData(instance);
    if (instance_data && instance_data->vtable.EnumeratePhysicalDevices) {
        VkResult result = instance_data->vtable.EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
        LogAPICall("vkEnumeratePhysicalDevices", "Enumeration completed");
        return result;
    }
    
    // Fallback to driver call
    PFN_vkEnumeratePhysicalDevices fpEnumerate = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    if (fpEnumerate) {
        VkResult result = fpEnumerate(instance, pPhysicalDeviceCount, pPhysicalDevices);
        LogAPICall("vkEnumeratePhysicalDevices", "Enumeration completed (fallback)");
        return result;
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    
    LogAPICall("vkGetPhysicalDeviceProperties", "Getting physical device properties");
    
    // For global functions like this, we need to find the instance
    std::lock_guard<std::mutex> lock(global_mutex);
    if (!instance_map.empty()) {
        InstanceData* instance_data = instance_map.begin()->second;
        if (instance_data && instance_data->vtable.GetPhysicalDeviceProperties) {
            instance_data->vtable.GetPhysicalDeviceProperties(physicalDevice, pProperties);
            LogAPICall("vkGetPhysicalDeviceProperties", "Properties retrieved");
            return;
        }
    }
    
    // Fallback - this is more complex for global functions
    LogAPICall("vkGetPhysicalDeviceProperties", "Using fallback method");
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName) {
    
    // Return our layer's functions
    if (strcmp(pName, "vkCreateInstance") == 0) return (PFN_vkVoidFunction)vkCreateInstance;
    if (strcmp(pName, "vkDestroyInstance") == 0) return (PFN_vkVoidFunction)vkDestroyInstance;
    if (strcmp(pName, "vkEnumeratePhysicalDevices") == 0) return (PFN_vkVoidFunction)vkEnumeratePhysicalDevices;
    if (strcmp(pName, "vkGetPhysicalDeviceProperties") == 0) return (PFN_vkVoidFunction)vkGetPhysicalDeviceProperties;
    if (strcmp(pName, "vkCreateDevice") == 0) return (PFN_vkVoidFunction)vkCreateDevice;
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    if (strcmp(pName, "vkEnumerateInstanceLayerProperties") == 0) return (PFN_vkVoidFunction)vkEnumerateInstanceLayerProperties;
    if (strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0) return (PFN_vkVoidFunction)vkEnumerateInstanceExtensionProperties;
    if (strcmp(pName, "vkEnumerateDeviceLayerProperties") == 0) return (PFN_vkVoidFunction)vkEnumerateDeviceLayerProperties;
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) return (PFN_vkVoidFunction)vkEnumerateDeviceExtensionProperties;
    
    // For other functions, get from next layer
    if (instance) {
        InstanceData* instance_data = GetInstanceData(instance);
        if (instance_data && instance_data->vtable.GetInstanceProcAddr) {
            return instance_data->vtable.GetInstanceProcAddr(instance, pName);
        }
    }
    
    return nullptr;
}

// Minimal implementations for other required functions
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    LogAPICall("vkCreateDevice", "Creating logical device");
    
    // Simple passthrough for now
    std::lock_guard<std::mutex> lock(global_mutex);
    if (!instance_map.empty()) {
        InstanceData* instance_data = instance_map.begin()->second;
        if (instance_data && instance_data->vtable.CreateDevice) {
            VkResult result = instance_data->vtable.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
            LogAPICall("vkCreateDevice", result == VK_SUCCESS ? "Device created successfully" : "Device creation failed");
            return result;
        }
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    LogAPICall("vkDestroyDevice", "Destroying logical device");
    // For now, just log - device destruction is typically handled by the driver
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName) {
    
    // Return our layer's functions
    if (strcmp(pName, "vkDestroyDevice") == 0) return (PFN_vkVoidFunction)vkDestroyDevice;
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    
    return nullptr;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties) {
    
    if (pProperties == nullptr) {
        *pPropertyCount = 1;
        return VK_SUCCESS;
    }
    
    if (*pPropertyCount >= 1) {
        memcpy(pProperties, &layer_props, sizeof(VkLayerProperties));
        *pPropertyCount = 1;
        return VK_SUCCESS;
    }
    
    return VK_INCOMPLETE;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    
    if (pLayerName && strcmp(pLayerName, LAYER_NAME) == 0) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }
    
    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties) {
    
    return vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    
    // Don't handle layer-specific queries for our layer
    if (pLayerName && strcmp(pLayerName, LAYER_NAME) == 0) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }
    
    // Forward to next layer/driver for extension enumeration
    std::lock_guard<std::mutex> lock(global_mutex);
    if (!instance_map.empty()) {
        InstanceData* instance_data = instance_map.begin()->second;
        if (instance_data && instance_data->vtable.GetInstanceProcAddr) {
            PFN_vkEnumerateDeviceExtensionProperties fpEnumerate = 
                (PFN_vkEnumerateDeviceExtensionProperties)instance_data->vtable.GetInstanceProcAddr(
                    instance_data->instance, "vkEnumerateDeviceExtensionProperties");
            if (fpEnumerate) {
                return fpEnumerate(physicalDevice, pLayerName, pPropertyCount, pProperties);
            }
        }
    }
    
    return VK_ERROR_LAYER_NOT_PRESENT;
}
