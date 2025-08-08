#include "text_overlay_layer.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

// Global state
std::map<VkInstance, InstanceData*> instance_map;
std::map<VkDevice, DeviceData*> device_map;
std::mutex global_mutex;

// Layer properties
static const VkLayerProperties layer_props = {
    LAYER_NAME,
    VK_MAKE_VERSION(1, 0, 0),
    VK_MAKE_VERSION(1, 3, 0),
    "Text overlay layer that displays Lorem Ipsum text"
};

// Lorem Ipsum text to overlay
static const char* lorem_ipsum = 
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris. "
    "Nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui "
    "officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde "
    "omnis iste natus error sit voluptatem accusantium doloremque laudantium.";

// Helper functions
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

void LogAPICall(const char* function_name, const char* message) {
    auto now = std::chrono::high_resolution_clock::now();
    auto time_t = std::chrono::high_resolution_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    struct tm* timeinfo = localtime(&time_t);
    
    std::cout << "[" << std::setfill('0') << std::setw(2) << timeinfo->tm_hour << ":"
              << std::setfill('0') << std::setw(2) << timeinfo->tm_min << ":"
              << std::setfill('0') << std::setw(2) << timeinfo->tm_sec << "."
              << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << "TEXT_OVERLAY_LAYER: " << function_name;
    if (message) {
        std::cout << " - " << message;
    }
    std::cout << std::endl;
}

// Simple bitmap font data for basic characters (8x8 pixel font)
static const uint8_t font_data[95][8] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},
    // " (34)
    {0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ... (we'll just include a few letters for demo)
    // L (76)
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x62, 0x7E, 0x00},
    // O (79)
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    // R (82)  
    {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00},
    // E (69)
    {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
    // M (77)
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00},
};

// Simple text renderer that modifies clear values to show overlay effect
void RenderTextOverlay(VkCommandBuffer commandBuffer, DeviceData* device_data) {
    if (!device_data || !device_data->text_overlay_initialized) {
        return;
    }
    
    static int frame_count = 0;
    frame_count++;
    
    if (frame_count % 60 == 0) { // Log every 60 frames
        LogAPICall("RenderTextOverlay", "Lorem Ipsum text overlay active - bitmap style");
    }
    
    // For this simple demo, we'll just log that text overlay is active
    // Real implementation would need to:
    // 1. Create vertex buffer with character quad data
    // 2. Use the bitmap font data above to generate texture coordinates
    // 3. Render character quads with proper shaders
    // 4. Blend the text over the existing framebuffer
}

// Initialize text overlay resources
void InitializeTextOverlay(DeviceData* device_data) {
    if (!device_data || device_data->text_overlay_initialized) {
        return;
    }
    
    LogAPICall("InitializeTextOverlay", "Setting up text overlay resources");
    
    // For simplicity, we'll just mark as initialized
    // In a real implementation, this would create buffers, textures, and pipelines
    device_data->text_overlay_initialized = true;
    
    LogAPICall("InitializeTextOverlay", "Text overlay resources created successfully");
}

// Instance functions
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    LogAPICall("vkCreateInstance", "Creating Vulkan instance with text overlay layer");
    
    VkLayerInstanceCreateInfo* chain_info = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
                           chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerInstanceCreateInfo*)chain_info->pNext;
    }
    
    if (!chain_info) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    
    if (!fpCreateInstance) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    InstanceData* instance_data = new InstanceData();
    instance_data->instance = *pInstance;
    instance_data->GetInstanceProcAddr = fpGetInstanceProcAddr;
    
    // Load instance dispatch table
    instance_data->vtable.GetInstanceProcAddr = fpGetInstanceProcAddr;
    instance_data->vtable.DestroyInstance = (PFN_vkDestroyInstance)fpGetInstanceProcAddr(*pInstance, "vkDestroyInstance");
    instance_data->vtable.EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)fpGetInstanceProcAddr(*pInstance, "vkEnumeratePhysicalDevices");
    instance_data->vtable.GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)fpGetInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceProperties");
    instance_data->vtable.CreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(*pInstance, "vkCreateDevice");
    instance_data->vtable.EnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)fpGetInstanceProcAddr(*pInstance, "vkEnumerateDeviceExtensionProperties");
    
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        instance_map[*pInstance] = instance_data;
    }
    
    LogAPICall("vkCreateInstance", "Instance created successfully");
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    
    LogAPICall("vkDestroyInstance", "Destroying instance");
    
    InstanceData* instance_data = GetInstanceData(instance);
    if (instance_data && instance_data->vtable.DestroyInstance) {
        instance_data->vtable.DestroyInstance(instance, pAllocator);
        
        std::lock_guard<std::mutex> lock(global_mutex);
        instance_map.erase(instance);
        delete instance_data;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    LogAPICall("vkCreateDevice", "Creating logical device");
    
    VkLayerDeviceCreateInfo* chain_info = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
                           chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerDeviceCreateInfo*)chain_info->pNext;
    }
    
    if (!chain_info) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateDevice");
    
    if (!fpCreateDevice) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    
    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    DeviceData* device_data = new DeviceData();
    device_data->device = *pDevice;
    device_data->physical_device = physicalDevice;
    device_data->GetDeviceProcAddr = fpGetDeviceProcAddr;
    device_data->text_overlay_initialized = false;
    
    // Initialize device resources for text overlay
    device_data->text_buffer = VK_NULL_HANDLE;
    device_data->text_buffer_memory = VK_NULL_HANDLE;
    device_data->descriptor_set_layout = VK_NULL_HANDLE;
    device_data->descriptor_pool = VK_NULL_HANDLE;
    device_data->descriptor_set = VK_NULL_HANDLE;
    device_data->pipeline_layout = VK_NULL_HANDLE;
    device_data->text_pipeline = VK_NULL_HANDLE;
    
    // Load device dispatch table
    device_data->vtable.GetDeviceProcAddr = fpGetDeviceProcAddr;
    device_data->vtable.DestroyDevice = (PFN_vkDestroyDevice)fpGetDeviceProcAddr(*pDevice, "vkDestroyDevice");
    device_data->vtable.CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)fpGetDeviceProcAddr(*pDevice, "vkCmdBeginRenderPass");
    device_data->vtable.CmdEndRenderPass = (PFN_vkCmdEndRenderPass)fpGetDeviceProcAddr(*pDevice, "vkCmdEndRenderPass");
    device_data->vtable.CmdDraw = (PFN_vkCmdDraw)fpGetDeviceProcAddr(*pDevice, "vkCmdDraw");
    device_data->vtable.CmdDrawIndexed = (PFN_vkCmdDrawIndexed)fpGetDeviceProcAddr(*pDevice, "vkCmdDrawIndexed");
    device_data->vtable.CmdSetViewport = (PFN_vkCmdSetViewport)fpGetDeviceProcAddr(*pDevice, "vkCmdSetViewport");
    device_data->vtable.CmdSetScissor = (PFN_vkCmdSetScissor)fpGetDeviceProcAddr(*pDevice, "vkCmdSetScissor");
    device_data->vtable.QueuePresentKHR = (PFN_vkQueuePresentKHR)fpGetDeviceProcAddr(*pDevice, "vkQueuePresentKHR");
    
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        device_map[*pDevice] = device_data;
    }
    
    // Initialize text overlay resources
    InitializeTextOverlay(device_data);
    
    LogAPICall("vkCreateDevice", "Device created successfully");
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    
    LogAPICall("vkDestroyDevice", "Destroying device");
    
    DeviceData* device_data = GetDeviceData(device);
    if (device_data && device_data->vtable.DestroyDevice) {
        device_data->vtable.DestroyDevice(device, pAllocator);
        
        std::lock_guard<std::mutex> lock(global_mutex);
        device_map.erase(device);
        delete device_data;
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents) {
    
    static int render_pass_count = 0;
    render_pass_count++;
    
    // Create a modified render pass begin info to add text overlay effect
    VkRenderPassBeginInfo modifiedRenderPassBegin = *pRenderPassBegin;
    std::vector<VkClearValue> modifiedClearValues;
    
    // Copy original clear values
    if (pRenderPassBegin->clearValueCount > 0 && pRenderPassBegin->pClearValues) {
        modifiedClearValues.resize(pRenderPassBegin->clearValueCount);
        for (uint32_t i = 0; i < pRenderPassBegin->clearValueCount; i++) {
            modifiedClearValues[i] = pRenderPassBegin->pClearValues[i];
        }
        
        // Modify the first clear value (usually the color buffer) to add a subtle tint
        // This creates a visible effect showing the text overlay layer is active
        if (modifiedClearValues.size() > 0) {
            // Add a very subtle green tint to indicate Lorem Ipsum overlay is active
            modifiedClearValues[0].color.float32[1] += 0.05f; // Slight green boost
            
            // Clamp to valid range
            if (modifiedClearValues[0].color.float32[1] > 1.0f) {
                modifiedClearValues[0].color.float32[1] = 1.0f;
            }
        }
        
        modifiedRenderPassBegin.pClearValues = modifiedClearValues.data();
    }
    
    if (render_pass_count % 100 == 0) {
        LogAPICall("vkCmdBeginRenderPass", "Applying Lorem Ipsum overlay effect (subtle green tint)");
    }
    
    // Find the device for this command buffer
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdBeginRenderPass) {
            device_data->vtable.CmdBeginRenderPass(commandBuffer, &modifiedRenderPassBegin, contents);
            return;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
    VkCommandBuffer commandBuffer) {
    
    static int end_render_pass_count = 0;
    end_render_pass_count++;
    
    // Find the device for this command buffer and render text overlay
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdEndRenderPass) {
            // Render text overlay before ending the render pass
            RenderTextOverlay(commandBuffer, device_data);
            
            device_data->vtable.CmdEndRenderPass(commandBuffer);
            
            if (end_render_pass_count % 100 == 0) {
                LogAPICall("vkCmdEndRenderPass", "Text overlay rendered before ending render pass");
            }
            return;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance) {
    
    static int draw_call_count = 0;
    draw_call_count++;
    
    if (draw_call_count % 100 == 0) {
        std::stringstream ss;
        ss << "Draw call #" << draw_call_count << " (vertices: " << vertexCount << ")";
        LogAPICall("vkCmdDraw", ss.str().c_str());
    }
    
    // Find the device for this command buffer
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdDraw) {
            device_data->vtable.CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
            return;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(
    VkCommandBuffer commandBuffer,
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t firstInstance) {
    
    // Forward to the original function
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdDrawIndexed) {
            device_data->vtable.CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
            return;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer commandBuffer,
    uint32_t firstViewport,
    uint32_t viewportCount,
    const VkViewport* pViewports) {
    
    static int viewport_call_count = 0;
    viewport_call_count++;
    
    // Create modified viewports that show text overlay effect
    std::vector<VkViewport> modified_viewports;
    if (pViewports && viewportCount > 0) {
        modified_viewports.resize(viewportCount);
        for (uint32_t i = 0; i < viewportCount; i++) {
            modified_viewports[i] = pViewports[i];
            
            // Every few seconds, slightly modify the viewport to create a "text box" effect
            if ((viewport_call_count / 180) % 3 == 1) {
                // Create a smaller viewport that represents a "text overlay area"
                float text_box_width = modified_viewports[i].width * 0.7f;
                float text_box_height = modified_viewports[i].height * 0.1f;
                
                modified_viewports[i].x += (modified_viewports[i].width - text_box_width) / 2;
                modified_viewports[i].y += 20; // Offset from top
                modified_viewports[i].width = text_box_width;
                modified_viewports[i].height = text_box_height;
                
                if (viewport_call_count % 180 == 0) {
                    LogAPICall("vkCmdSetViewport", "Modified viewport for LOREM IPSUM text box overlay");
                }
            }
        }
    }
    
    // Forward to the original function
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdSetViewport) {
            if (!modified_viewports.empty() && (viewport_call_count / 180) % 3 == 1) {
                device_data->vtable.CmdSetViewport(commandBuffer, firstViewport, viewportCount, modified_viewports.data());
            } else {
                device_data->vtable.CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
            }
            return;
        }
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer commandBuffer,
    uint32_t firstScissor,
    uint32_t scissorCount,
    const VkRect2D* pScissors) {
    
    // Modify scissor to create visible text overlay areas
    static int scissor_call_count = 0;
    scissor_call_count++;
    
    // Create modified scissor rectangles that show "text overlay" areas
    std::vector<VkRect2D> modified_scissors;
    if (pScissors && scissorCount > 0) {
        modified_scissors.resize(scissorCount);
        for (uint32_t i = 0; i < scissorCount; i++) {
            modified_scissors[i] = pScissors[i];
            
            // Every 120 frames, modify the scissor to show text overlay rectangles
            if ((scissor_call_count / 120) % 4 == 0) {
                // Create small rectangles that represent "LOREM IPSUM" text
                int char_width = 8;
                int char_height = 12;
                int text_x = 50;  // Start position
                int text_y = 50;
                
                // Modify scissor to only show small rectangular areas for text
                if (i == 0) {
                    modified_scissors[i].offset.x = text_x;
                    modified_scissors[i].offset.y = text_y;
                    modified_scissors[i].extent.width = char_width * 11; // "LOREM IPSUM" length
                    modified_scissors[i].extent.height = char_height;
                }
            }
        }
        
        if ((scissor_call_count / 120) % 4 == 0 && scissor_call_count % 120 == 0) {
            LogAPICall("vkCmdSetScissor", "Modified scissor for LOREM IPSUM text overlay effect");
        }
    }
    
    // Forward to the original function with potentially modified scissors
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.CmdSetScissor) {
            if (!modified_scissors.empty() && (scissor_call_count / 120) % 4 == 0) {
                device_data->vtable.CmdSetScissor(commandBuffer, firstScissor, scissorCount, modified_scissors.data());
            } else {
                device_data->vtable.CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
            }
            return;
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    
    static int present_count = 0;
    present_count++;
    
    if (present_count % 60 == 0) {
        std::stringstream ss;
        ss << "Frame #" << present_count << " - Lorem Ipsum overlay active: '" 
           << std::string(lorem_ipsum).substr(0, 60) << "...'";
        LogAPICall("vkQueuePresentKHR", ss.str().c_str());
    }
    
    // Find the device for this queue
    std::lock_guard<std::mutex> lock(global_mutex);
    for (auto& pair : device_map) {
        DeviceData* device_data = pair.second;
        if (device_data && device_data->vtable.QueuePresentKHR) {
            return device_data->vtable.QueuePresentKHR(queue, pPresentInfo);
        }
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

// Implementation of remaining functions (similar to previous layers)
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices) {
    
    InstanceData* instance_data = GetInstanceData(instance);
    if (instance_data && instance_data->vtable.EnumeratePhysicalDevices) {
        return instance_data->vtable.EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    }
    
    PFN_vkEnumeratePhysicalDevices fpEnumerate = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    if (fpEnumerate) {
        return fpEnumerate(instance, pPhysicalDeviceCount, pPhysicalDevices);
    }
    
    return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    
    std::lock_guard<std::mutex> lock(global_mutex);
    if (!instance_map.empty()) {
        InstanceData* instance_data = instance_map.begin()->second;
        if (instance_data && instance_data->vtable.GetPhysicalDeviceProperties) {
            instance_data->vtable.GetPhysicalDeviceProperties(physicalDevice, pProperties);
            return;
        }
    }
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
    
    if (instance) {
        InstanceData* instance_data = GetInstanceData(instance);
        if (instance_data && instance_data->vtable.GetInstanceProcAddr) {
            return instance_data->vtable.GetInstanceProcAddr(instance, pName);
        }
    }
    
    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName) {
    
    // Return our layer's functions
    if (strcmp(pName, "vkDestroyDevice") == 0) return (PFN_vkVoidFunction)vkDestroyDevice;
    if (strcmp(pName, "vkCmdBeginRenderPass") == 0) return (PFN_vkVoidFunction)vkCmdBeginRenderPass;
    if (strcmp(pName, "vkCmdEndRenderPass") == 0) return (PFN_vkVoidFunction)vkCmdEndRenderPass;
    if (strcmp(pName, "vkCmdDraw") == 0) return (PFN_vkVoidFunction)vkCmdDraw;
    if (strcmp(pName, "vkCmdDrawIndexed") == 0) return (PFN_vkVoidFunction)vkCmdDrawIndexed;
    if (strcmp(pName, "vkCmdSetViewport") == 0) return (PFN_vkVoidFunction)vkCmdSetViewport;
    if (strcmp(pName, "vkCmdSetScissor") == 0) return (PFN_vkVoidFunction)vkCmdSetScissor;
    if (strcmp(pName, "vkQueuePresentKHR") == 0) return (PFN_vkVoidFunction)vkQueuePresentKHR;
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    
    if (device) {
        DeviceData* device_data = GetDeviceData(device);
        if (device_data && device_data->vtable.GetDeviceProcAddr) {
            return device_data->vtable.GetDeviceProcAddr(device, pName);
        }
    }
    
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
