// Copyright (C) 2018 David Reid. See included LICENSE file.

#include "ocGraphics_Vulkan_Autogen.cpp"

// SOME NOTES ON VULKAN
//
// - Positive Y = DOWN (Opposite to OpenGL)
//   - Means back faces are now the front with counter-clockwise polygon winding
//   - Can make this consistent by multiplying the projection matrix with an adjustment matrix: ocMakeMat4_VulkanClipCorrection()
// - Depth range is [-1,1] (OpenGL is [0,1])
//   - Can also make this consistent with ocMakeMat4_VulkanClipCorrection()

///////////////////////////////////////////////////////////////////////////////
//
// GraphicsContext
//
///////////////////////////////////////////////////////////////////////////////

OC_PRIVATE VkFormat ocToVulkanImageFormat(ocImageFormat format)
{
    switch (format) {
        case ocImageFormat_R8G8B8A8:      return VK_FORMAT_R8G8B8A8_UNORM;
        case ocImageFormat_SRGBA8:        return VK_FORMAT_R8G8B8A8_SRGB;
        case ocImageFormat_R16G16B16A16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
        default:                          return VK_FORMAT_UNDEFINED;
    }
}

#if 0
OC_PRIVATE VkPrimitiveTopology ocToVulkanPrimitiveType(ocGraphicsPrimitiveType primitiveType)
{
    switch (primitiveType) {
        case ocGraphicsPrimitiveType_Point:    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case ocGraphicsPrimitiveType_Line:     return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case ocGraphicsPrimitiveType_Triangle: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}
#endif

OC_PRIVATE VkIndexType ocToVulkanIndexFormat(ocGraphicsIndexFormat indexFormat)
{
    switch (indexFormat) {
        case ocGraphicsIndexFormat_UInt16: return VK_INDEX_TYPE_UINT16;
        case ocGraphicsIndexFormat_UInt32: return VK_INDEX_TYPE_UINT32;
        default: return VK_INDEX_TYPE_UINT32;
    }
}

OC_PRIVATE VkPresentModeKHR ocGraphicsGetBestPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, ocVSyncMode vsync)
{
    VkPresentModeKHR supportedPresentModes[4 /*VK_PRESENT_MODE_RANGE_SIZE_KHR*/]; // TODO: Need to update dr_vulkan.h
    uint32_t supportedPresentModesCount = sizeof(supportedPresentModes) / sizeof(supportedPresentModes[0]);
    VkResult vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportedPresentModesCount, supportedPresentModes);
    if (vkResult != VK_SUCCESS) {
        return VK_PRESENT_MODE_FIFO_KHR;    // <-- All implementations of this extension must support this.
    }

    VkPresentModeKHR modes[4];
    switch (vsync) {
        case ocVSyncMode_Disabled:
        {
            modes[0] = VK_PRESENT_MODE_IMMEDIATE_KHR;
            modes[1] = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            modes[2] = VK_PRESENT_MODE_MAILBOX_KHR;
            modes[3] = VK_PRESENT_MODE_FIFO_KHR;
        } break;

        case ocVSyncMode_Enabled:
        {
            modes[0] = VK_PRESENT_MODE_FIFO_KHR;
            modes[1] = VK_PRESENT_MODE_MAILBOX_KHR;
            modes[2] = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            modes[3] = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } break;
        
        case ocVSyncMode_Adaptive:
        default:
        {
            modes[0] = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            modes[1] = VK_PRESENT_MODE_MAILBOX_KHR;
            modes[2] = VK_PRESENT_MODE_FIFO_KHR;
            modes[3] = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } break;
    }

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        for (size_t j = 0; j < supportedPresentModesCount; ++j) {
            if (modes[i] == supportedPresentModes[j]) {
                return supportedPresentModes[j];
            }
        }
    }

    // Should never actually get here, but for sanity just return VK_PRESENT_MODE_FIFO_KHR since it should always be supported.
    return VK_PRESENT_MODE_FIFO_KHR;
}

OC_PRIVATE ocResult ocToResultFromVulkan(VkResult vkresult)
{
    // TODO: Implement me fully.
    switch (vkresult)
    {
        case VK_SUCCESS: return OC_SUCCESS;
        default: return OC_ERROR;
    }
}


OC_PRIVATE VkComponentMapping ocvkDefaultComponentMapping()
{
    VkComponentMapping result;
    result.r = VK_COMPONENT_SWIZZLE_R;
    result.g = VK_COMPONENT_SWIZZLE_G;
    result.b = VK_COMPONENT_SWIZZLE_B;
    result.a = VK_COMPONENT_SWIZZLE_A;
    return result;
}

OC_PRIVATE VkImageSubresourceRange ocvkImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
{
    VkImageSubresourceRange result;
    result.aspectMask = aspectMask;
    result.baseMipLevel = baseMipLevel;
    result.levelCount = levelCount;
    result.baseArrayLayer = baseArrayLayer;
    result.layerCount = layerCount;
    return result;
}

OC_PRIVATE VkExtent3D ocvkExtent3D(uint32_t width, uint32_t height, uint32_t depth)
{
    VkExtent3D result;
    result.width = width;
    result.height = height;
    result.depth = depth;
    return result;
}

OC_PRIVATE VkRect2D ocvkRect2D(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    VkRect2D result;
    result.offset.x = x;
    result.offset.y = y;
    result.extent.width = width;
    result.extent.height = height;
    return result;
}

#if 0
OC_PRIVATE VkClearColorValue ocvkClearColorValue4f(float r, float g, float b, float a)
{
    VkClearColorValue result;
    result.float32[0] = r;
    result.float32[1] = g;
    result.float32[2] = b;
    result.float32[3] = a;
    return result;
}

OC_PRIVATE VkClearColorValue ocvkClearColorValue4i(int32_t r, int32_t g, int32_t b, int32_t a)
{
    VkClearColorValue result;
    result.int32[0] = r;
    result.int32[1] = g;
    result.int32[2] = b;
    result.int32[3] = a;
    return result;
}

OC_PRIVATE VkClearColorValue ocvkClearColorValue4ui(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    VkClearColorValue result;
    result.uint32[0] = r;
    result.uint32[1] = g;
    result.uint32[2] = b;
    result.uint32[3] = a;
    return result;
}
#endif

OC_PRIVATE VkClearValue ocvkClearValueColor4f(float r, float g, float b, float a)
{
    VkClearValue result;
    result.color.float32[0] = r;
    result.color.float32[1] = g;
    result.color.float32[2] = b;
    result.color.float32[3] = a;
    return result;
}

#if 0
OC_PRIVATE VkClearValue ocvkClearValueColor4i(int32_t r, int32_t g, int32_t b, int32_t a)
{
    VkClearValue result;
    result.color.int32[0] = r;
    result.color.int32[1] = g;
    result.color.int32[2] = b;
    result.color.int32[3] = a;
    return result;
}

OC_PRIVATE VkClearValue ocvkClearValueColor4ui(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    VkClearValue result;
    result.color.uint32[0] = r;
    result.color.uint32[1] = g;
    result.color.uint32[2] = b;
    result.color.uint32[3] = a;
    return result;
}
#endif

OC_PRIVATE VkClearValue ocvkClearValueDepthStencil(float depth, uint32_t stencil)
{
    VkClearValue result;
    result.depthStencil.depth = depth;
    result.depthStencil.stencil = stencil;
    return result;
}

OC_PRIVATE VkViewport ocvkViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    VkViewport result;
    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;
    result.minDepth = minDepth;
    result.maxDepth = maxDepth;
    return result;
}

OC_PRIVATE VkPipelineShaderStageCreateInfo ocvkPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName, VkSpecializationInfo* pSpecializationInfo)
{
    VkPipelineShaderStageCreateInfo result;
    result.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    result.pNext = NULL;
    result.flags = 0;
    result.stage = stage;
    result.module = module;
    result.pName = pName;
    result.pSpecializationInfo = pSpecializationInfo;
    return result;
}

OC_PRIVATE VkImageCreateInfo ocvkSimpleImageCreateInfo2D(VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, VkImageUsageFlags usage)
{
    VkImageCreateInfo result;
    result.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    result.pNext = NULL;
    result.flags = 0;
    result.imageType = VK_IMAGE_TYPE_2D;
    result.format = format;
    result.extent.width = width;
    result.extent.height = height;
    result.extent.depth = 1;
    result.mipLevels = mipLevels;
    result.arrayLayers = 1;
    result.samples = VK_SAMPLE_COUNT_1_BIT;
    result.tiling = VK_IMAGE_TILING_OPTIMAL;
    result.usage = usage;
    result.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    result.queueFamilyIndexCount = 0;
    result.pQueueFamilyIndices = NULL;
    result.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return result;
}

OC_PRIVATE VkResult ocvkCreateSimpleImageView(VkDevice device, VkImage image, VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    VkImageViewCreateInfo viewInfo;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = NULL;
    viewInfo.flags = 0;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.components = ocvkDefaultComponentMapping();
    viewInfo.subresourceRange = subresourceRange;
    return vkCreateImageView(device, &viewInfo, pAllocator, pView);
}

OC_PRIVATE VkResult ocvkAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t commandBufferCount, VkCommandBuffer* pCommandBuffers)
{
    VkCommandBufferAllocateInfo info;
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = NULL;
    info.commandPool        = commandPool;
    info.level              = level;
    info.commandBufferCount = commandBufferCount;
    return vkAllocateCommandBuffers(device, &info, pCommandBuffers);
}

OC_PRIVATE VkResult ocvkBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* pInheritanceInfo)
{
    VkCommandBufferBeginInfo info;
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext            = NULL;
    info.flags            = flags;
    info.pInheritanceInfo = pInheritanceInfo;
    return vkBeginCommandBuffer(commandBuffer, &info);
}

OC_PRIVATE void ocvkFilterUnsupportedLayers(uint32_t* pLayerCount, const char** ppLayers)
{
    if (pLayerCount == NULL || ppLayers == NULL) return;

    VkLayerProperties supportedLayers[64];
    uint32_t supportedLayerCount = sizeof(supportedLayers) / sizeof(supportedLayers[0]);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers);

    for (uint32_t iDesiredLayer = 0; iDesiredLayer < *pLayerCount; /* DO NOTHING */) {
        VkBool32 isSupported = VK_FALSE;
        for (uint32_t iSupportedLayer = 0; iSupportedLayer < supportedLayerCount; ++iSupportedLayer) {
            if (strcmp(supportedLayers[iSupportedLayer].layerName, ppLayers[iDesiredLayer]) == 0) {
                isSupported = VK_TRUE;
                break;
            }
        }

        if (!isSupported) {
            for (uint32_t j = iDesiredLayer; j < *pLayerCount-1; ++j) {
                ppLayers[j] = ppLayers[j+1];
            }

            *pLayerCount -= 1;
        } else {
            iDesiredLayer += 1;
        }
    }
}

OC_PRIVATE void ocvkFilterUnsupportedInstanceExtensions(const char* pLayerName, uint32_t* pExtensionCount, const char** ppExtensions)
{
    if (pExtensionCount == NULL || ppExtensions == NULL) {
        return;
    }

    VkExtensionProperties supportedExtensions[256];
    uint32_t supportedExtensionCount = sizeof(supportedExtensions) / sizeof(supportedExtensions[0]);
    vkEnumerateInstanceExtensionProperties(pLayerName, &supportedExtensionCount, supportedExtensions);

    for (uint32_t iDesiredExtension = 0; iDesiredExtension < *pExtensionCount; /* DO NOTHING */) {
        VkBool32 isSupported = VK_FALSE;
        for (uint32_t iSupportedExtension = 0; iSupportedExtension < supportedExtensionCount; ++iSupportedExtension) {
            if (strcmp(supportedExtensions[iSupportedExtension].extensionName, ppExtensions[iDesiredExtension]) == 0) {
                isSupported = VK_TRUE;
                break;
            }
        }

        if (!isSupported) {
            for (uint32_t j = iDesiredExtension; j < *pExtensionCount-1; ++j) {
                ppExtensions[j] = ppExtensions[j+1];
            }

            *pExtensionCount -= 1;
        } else {
            iDesiredExtension += 1;
        }
    }
}

OC_PRIVATE void ocvkFilterUnsupportedDeviceExtensions(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pExtensionCount, const char** ppExtensions)
{
    if (pExtensionCount == NULL || ppExtensions == NULL) {
        return;
    }

    VkExtensionProperties supportedExtensions[256];
    uint32_t supportedExtensionCount = sizeof(supportedExtensions) / sizeof(supportedExtensions[0]);
    vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &supportedExtensionCount, supportedExtensions);

    for (uint32_t iDesiredExtension = 0; iDesiredExtension < *pExtensionCount; /* DO NOTHING */) {
        VkBool32 isSupported = VK_FALSE;
        for (uint32_t iSupportedExtension = 0; iSupportedExtension < supportedExtensionCount; ++iSupportedExtension) {
            if (strcmp(supportedExtensions[iSupportedExtension].extensionName, ppExtensions[iDesiredExtension]) == 0) {
                isSupported = VK_TRUE;
                break;
            }
        }

        if (!isSupported) {
            for (uint32_t j = iDesiredExtension; j < *pExtensionCount-1; ++j) {
                ppExtensions[j] = ppExtensions[j+1];
            }

            *pExtensionCount -= 1;
        } else {
            iDesiredExtension += 1;
        }
    }
}

OC_PRIVATE VkResult ocvkCreateShaderModule(VkDevice device, size_t codeSize, const uint32_t* pCode, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
    VkShaderModuleCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.codeSize = codeSize;
    info.pCode = pCode;
    return vkCreateShaderModule(device, &info, pAllocator, pShaderModule);
}

OC_PRIVATE uint32_t ocvkGetMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);

    for (uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1 << i)) && (memoryProps.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i;
        }
    }

    return 0xFFFFFFFF;  // Error.
}

OC_PRIVATE VkResult ocvkAllocateAndBindImageMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkImage image, VkMemoryPropertyFlags memoryPropertyFlags, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    VkMemoryRequirements memreqs;
    vkGetImageMemoryRequirements(device, image, &memreqs);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.allocationSize = memreqs.size;
    allocInfo.memoryTypeIndex = ocvkGetMemoryTypeIndex(physicalDevice, memreqs.memoryTypeBits, memoryPropertyFlags);
    VkResult result = vkAllocateMemory(device, &allocInfo, pAllocator, pMemory);
    if (result != VK_SUCCESS) {
        return result;
    }

    return vkBindImageMemory(device, image, *pMemory, 0);
}

OC_PRIVATE VkResult ocvkAllocateAndBindBufferMemory(VkPhysicalDevice physicalDevice, VkDevice device, VkBuffer buffer, VkMemoryPropertyFlags memoryPropertyFlags, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    VkMemoryRequirements memreqs;
    vkGetBufferMemoryRequirements(device, buffer, &memreqs);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.allocationSize = memreqs.size;
    allocInfo.memoryTypeIndex = ocvkGetMemoryTypeIndex(physicalDevice, memreqs.memoryTypeBits, memoryPropertyFlags);
    VkResult result = vkAllocateMemory(device, &allocInfo, pAllocator, pMemory);
    if (result != VK_SUCCESS) {
        return result;
    }

    return vkBindBufferMemory(device, buffer, *pMemory, 0);
}

OC_PRIVATE VkResult ocvkFlushMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
{
    VkMappedMemoryRange memoryRange;
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.pNext = NULL;
    memoryRange.memory = memory;
    memoryRange.offset = offset;
    memoryRange.size = size;
    return vkFlushMappedMemoryRanges(device, 1, &memoryRange);
}

OC_PRIVATE VkResult ocvkCreateStagingBuffer(VkPhysicalDevice physicalDevice, VkDevice device, size_t dataSize, const void* pData, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer, VkDeviceMemory* pMemory)
{
    *pBuffer = NULL;
    *pMemory = NULL;

    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = NULL;
    bufferInfo.flags = 0;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = NULL;

    VkBuffer buffer;
    VkResult result = vkCreateBuffer(device, &bufferInfo, pAllocator, &buffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkDeviceMemory memory;
    result = ocvkAllocateAndBindBufferMemory(physicalDevice, device, buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pAllocator, &memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, pAllocator);
        return result;
    }

    // Set the data if we have some.
    if (pData != NULL) {
        void* pInternalData;
        result = vkMapMemory(device, memory, 0, dataSize, 0, &pInternalData);
        if (result != VK_SUCCESS) {
            vkFreeMemory(device, memory, pAllocator);
            vkDestroyBuffer(device, buffer, pAllocator);
            return result;
        }

        memcpy(pInternalData, pData, dataSize);

        // My intuition tells me that vkUnmapMemory() should do an implicit flush, however I am unable to find any documentation that
        // confirms this behaviour. If I try to flush the memory _after_ unmapping, I get an error from the validation layer. Doing it
        // _before_ unmapping makes the error go away.
        ocvkFlushMemory(device, memory, 0, dataSize);
        vkUnmapMemory(device, memory);  // Is this an implicit flush?
    }


    *pBuffer = buffer;
    *pMemory = memory;
    return VK_SUCCESS;
}


OC_PRIVATE VkResult ocGraphicsDoInitialImageLayoutTransition(ocGraphicsContext* pGraphics, VkImage image, VkImageSubresourceRange subresourceRange, VkAccessFlags dstAccessMask, VkImageLayout newLayout)
{
    VkCommandBuffer cmdbuffer;
    VkResult vkresult = ocvkAllocateCommandBuffers(pGraphics->device, pGraphics->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &cmdbuffer);
    if (vkresult != VK_SUCCESS) {
        return vkresult;
    }

    ocvkBeginCommandBuffer(cmdbuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, NULL);
    {
        VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = pGraphics->queueFamilyIndex;
        barrier.dstQueueFamilyIndex = pGraphics->queueFamilyIndex;
        barrier.image = image;
        barrier.subresourceRange = subresourceRange;
        vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    vkEndCommandBuffer(cmdbuffer);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(pGraphics->queue, 1, &submitInfo, 0);
    vkQueueWaitIdle(pGraphics->queue);

    vkFreeCommandBuffers(pGraphics->device, pGraphics->commandPool, 1, &cmdbuffer);

    return VK_SUCCESS;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
OC_PRIVATE VkResult ocvkCreateWin32Surface(VkInstance instance, HWND hWnd, HINSTANCE hWin32Instance, VkSurfaceKHR* pSurface)
{
    if (hWin32Instance == NULL) {
        hWin32Instance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    }

    VkWin32SurfaceCreateInfoKHR surfaceInfo;
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.pNext = NULL;
    surfaceInfo.flags = 0;
    surfaceInfo.hinstance = hWin32Instance;
    surfaceInfo.hwnd = hWnd;
    return vkCreateWin32SurfaceKHR(instance, &surfaceInfo, NULL, pSurface);
}
#endif

OC_PRIVATE VkResult ocvkCreateSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t imageCount, VkSurfaceFormatKHR imageFormat, VkImageUsageFlags imageUsage, VkPresentModeKHR presentMode, VkSwapchainKHR oldSwapchain, VkSwapchainKHR* pSwapchain)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSwapchainCreateInfoKHR swapchainInfo;
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = NULL;
    swapchainInfo.flags = 0;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = imageFormat.format;
    swapchainInfo.imageColorSpace = imageFormat.colorSpace;
    swapchainInfo.imageExtent = surfaceCaps.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = imageUsage;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = NULL;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = oldSwapchain;
    return vkCreateSwapchainKHR(device, &swapchainInfo, NULL, pSwapchain);
}

OC_PRIVATE VkSurfaceFormatKHR ocvkChooseSurfaceImageFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t desiredFormatsCount, VkFormat* pDesiredFormats)
{
    VkSurfaceFormatKHR pSupportedFormats[1024];
    uint32_t supportedFormatsCount = sizeof(pSupportedFormats) / sizeof(pSupportedFormats[0]);
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportedFormatsCount, pSupportedFormats);
    if (result != VK_SUCCESS) {
        VkSurfaceFormatKHR format;
        format.format = VK_FORMAT_UNDEFINED;
        format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        return format;
    }

    uint32_t iFormat = (uint32_t)-1;
    for (uint32_t iDesiredFormat = 0; iDesiredFormat < desiredFormatsCount; ++iDesiredFormat) {
        for (uint32_t iSupportedFormat = 0; iSupportedFormat < supportedFormatsCount; ++iSupportedFormat) {
            if (pSupportedFormats[iSupportedFormat].format == pDesiredFormats[iDesiredFormat]) {
                iFormat = iSupportedFormat;
                break;
            }
        }
    }

    return pSupportedFormats[iFormat];
}

OC_PRIVATE VkResult ocvkCreateSemaphore(VkDevice device, VkSemaphore* pSemaphore)
{
    VkSemaphoreCreateInfo semInfo;
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semInfo.pNext = NULL;
    semInfo.flags = 0;
    return vkCreateSemaphore(device, &semInfo, NULL, pSemaphore);
}


OC_PRIVATE ocResult ocGraphicsInit_VulkanInstance(ocGraphicsContext* pGraphics)
{
    ocAssert(pGraphics != NULL);

    if (vkbInit(&pGraphics->vk) != VK_SUCCESS) {
        return OC_FAILED_TO_INIT_GRAPHICS;
    }

    // Layers.
#ifdef OC_DEBUG
    const char* ppEnabledLayerNames[] = {
         "VK_LAYER_GOOGLE_threading",
         "VK_LAYER_LUNARG_param_checker",
         "VK_LAYER_LUNARG_device_limits",
         "VK_LAYER_LUNARG_object_tracker",
         "VK_LAYER_LUNARG_image",
         "VK_LAYER_LUNARG_core_validation",
         "VK_LAYER_LUNARG_swapchain",
         "VK_LAYER_GOOGLE_unique_objects"
    };

    uint32_t enabledLayerCount = sizeof(ppEnabledLayerNames) / sizeof(ppEnabledLayerNames[0]);
    ocvkFilterUnsupportedLayers(&enabledLayerCount, ppEnabledLayerNames);
#else
    uint32_t enabledLayerCount = 0;
    const char** ppEnabledLayerNames = NULL;
#endif

    
    // Extensions.
    const char* ppEnabledExtensionNames[] =  {
        VK_KHR_SURFACE_EXTENSION_NAME
#ifdef VK_USE_PLATFORM_WIN32_KHR
      , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#ifdef OC_DEBUG
      , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
    };

    uint32_t enabledExtensionCount = sizeof(ppEnabledExtensionNames) / sizeof(ppEnabledExtensionNames[0]);
    ocvkFilterUnsupportedInstanceExtensions(NULL, &enabledExtensionCount, ppEnabledExtensionNames);

#if OC_LOG_LEVEL >= OC_LOG_LEVEL_VERBOSE
    ocLog(pGraphics->pEngine, "Extensions:");
    for (uint32_t i = 0; i < enabledExtensionCount; ++i) {
        ocLogf(pGraphics->pEngine, "    %s", ppEnabledExtensionNames[i]);
    }
#endif

    VkApplicationInfo applicationInfo;
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = "Open Chernobyl";
    applicationInfo.applicationVersion = 1;
    applicationInfo.pEngineName = "OCEngine";
    applicationInfo.engineVersion = 1;
    applicationInfo.apiVersion = 0;

    VkInstanceCreateInfo vkInstanceInfo;
    vkInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkInstanceInfo.pNext = NULL;
    vkInstanceInfo.flags = 0;
    vkInstanceInfo.pApplicationInfo = &applicationInfo;
    vkInstanceInfo.enabledLayerCount = enabledLayerCount;
    vkInstanceInfo.ppEnabledLayerNames = ppEnabledLayerNames;
    vkInstanceInfo.enabledExtensionCount = enabledExtensionCount;
    vkInstanceInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
    if (vkCreateInstance(&vkInstanceInfo, NULL, &pGraphics->instance) != VK_SUCCESS) {
        vkbUninit();
        return OC_FAILED_TO_INIT_GRAPHICS;
    }

    if (vkbInitInstanceAPI(pGraphics->instance, &pGraphics->vk) != VK_SUCCESS) {
        vkDestroyInstance(pGraphics->instance, NULL);
        vkbUninit();
        return OC_FAILED_TO_INIT_GRAPHICS;
    }

    // Bind the API to global scope for now. 
    vkbBindAPI(&pGraphics->vk);

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_VulkanDevices(ocGraphicsContext* pGraphics, uint32_t desiredMSAASamples)
{
    ocAssert(pGraphics != NULL);

    // For now we're only using one device, that device being the first one iterated.
    // TODO: Make this more robust? Is the first device always a good default?
    uint32_t vkPhysicalDeviceCount = 1;
    VkResult result = vkEnumeratePhysicalDevices(pGraphics->instance, &vkPhysicalDeviceCount, &pGraphics->physicalDevice);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        return ocToResultFromVulkan(result);
    }


    // Queue family. We just need a family supporting graphics. The local queue index is always 0 for the moment.
    VkQueueFamilyProperties queueFamilyProperties[16];
    uint32_t queueFamilyCount = sizeof(queueFamilyProperties) / sizeof(queueFamilyProperties[0]);
    vkGetPhysicalDeviceQueueFamilyProperties(pGraphics->physicalDevice, &queueFamilyCount, queueFamilyProperties);
    
    pGraphics->queueFamilyIndex = (uint32_t)-1;
    for (uint32_t iQueueFamily = 0; iQueueFamily < queueFamilyCount; ++iQueueFamily) {
        if ((queueFamilyProperties[iQueueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            pGraphics->queueFamilyIndex = iQueueFamily;
            break;
        }
    }

    if (pGraphics->queueFamilyIndex == (uint32_t)-1) {
        return OC_FAILED_TO_INIT_GRAPHICS;
    }

    pGraphics->queueLocalIndex = 0;

    float queuePriority = 1;

    VkDeviceQueueCreateInfo queueInfo;
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = NULL;
    queueInfo.flags = 0;
    queueInfo.queueFamilyIndex = pGraphics->queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(pGraphics->physicalDevice, &physicalDeviceFeatures);

    const char* ppEnabledDeviceExtensionNames[] =  {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if 0
      , "VK_NV_glsl_shader"
#endif
    };

    uint32_t enabledDeviceExtensionCount = sizeof(ppEnabledDeviceExtensionNames) / sizeof(ppEnabledDeviceExtensionNames[0]);
    ocvkFilterUnsupportedDeviceExtensions(pGraphics->physicalDevice, NULL, &enabledDeviceExtensionCount, ppEnabledDeviceExtensionNames);

    VkDeviceCreateInfo deviceInfo;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = NULL;
    deviceInfo.flags = 0;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = NULL;
    deviceInfo.enabledExtensionCount = enabledDeviceExtensionCount;
    deviceInfo.ppEnabledExtensionNames = ppEnabledDeviceExtensionNames;
    deviceInfo.pEnabledFeatures = &physicalDeviceFeatures;
    result = vkCreateDevice(pGraphics->physicalDevice, &deviceInfo, NULL, &pGraphics->device);
    if (result != VK_SUCCESS) {
        return OC_FAILED_TO_INIT_GRAPHICS;
    }


    // Command pools. For the moment there is only a single command pool, but this will likely increase as we add
    // support for multi-threading.
    VkCommandPoolCreateInfo cmdpoolInfo;
    cmdpoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdpoolInfo.pNext = NULL;
    cmdpoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdpoolInfo.queueFamilyIndex = pGraphics->queueFamilyIndex;
    result = vkCreateCommandPool(pGraphics->device, &cmdpoolInfo, NULL, &pGraphics->commandPool);
    if (result != VK_SUCCESS) {
        vkDestroyDevice(pGraphics->device, NULL);
        return OC_FAILED_TO_INIT_GRAPHICS;
    }


    // Grab the queues.
    vkGetDeviceQueue(pGraphics->device, pGraphics->queueFamilyIndex, pGraphics->queueLocalIndex, &pGraphics->queue);


    // Clamp the desired MSAA level to a supported value.
    vkGetPhysicalDeviceProperties(pGraphics->physicalDevice, &pGraphics->deviceProps);
    uint32_t maxMSAAColor   = ocMaskMSB32((uint32_t)pGraphics->deviceProps.limits.framebufferColorSampleCounts);
    uint32_t maxMSAADepth   = ocMaskMSB32((uint32_t)pGraphics->deviceProps.limits.framebufferDepthSampleCounts);
    uint32_t maxMSAAStencil = ocMaskMSB32((uint32_t)pGraphics->deviceProps.limits.framebufferStencilSampleCounts);

    pGraphics->minMSAA = 1;
    pGraphics->maxMSAA = ocMin(maxMSAAColor, ocMin(maxMSAADepth, maxMSAAStencil));
    pGraphics->msaaSamples = (VkSampleCountFlagBits)ocClamp(desiredMSAASamples, pGraphics->minMSAA, pGraphics->maxMSAA);

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_VulkanRenderPass_FinalComposite(ocGraphicsContext* pGraphics, VkImageLayout outputImageFinalLayout, VkRenderPass* pRenderPass)
{
    ocAssert(pGraphics != NULL);

    VkAttachmentDescription attachments[2];

    // Output image.
    attachments[0].flags = 0;
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = outputImageFinalLayout;

    // Input image.
    attachments[1].flags = 0;
    attachments[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[1].samples = pGraphics->msaaSamples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //VkAttachmentReference pInputAttachments[1];
    //pInputAttachments[0].attachment = 0;
    //pInputAttachments[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentReferences[1];
    colorAttachmentReferences[0].attachment = 1;
    colorAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference pResolveAttachments[ocCountOf(colorAttachmentReferences)];
    pResolveAttachments[0].attachment = 0;
    pResolveAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    uint32_t pPreserveAttachments[1];
    pPreserveAttachments[0] = 0;

    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0; //ocCountOf(pInputAttachments);
    subpasses[0].pInputAttachments = NULL; //pInputAttachments;
    subpasses[0].colorAttachmentCount = ocCountOf(colorAttachmentReferences);
    subpasses[0].pColorAttachments = colorAttachmentReferences;
    subpasses[0].pResolveAttachments = pResolveAttachments;
    subpasses[0].pDepthStencilAttachment = NULL;
    subpasses[0].preserveAttachmentCount = ocCountOf(pPreserveAttachments);
    subpasses[0].pPreserveAttachments = pPreserveAttachments;

    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = NULL;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = ocCountOf(attachments);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = ocCountOf(subpasses);
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;
    VkResult vkresult = vkCreateRenderPass(pGraphics->device, &renderPassInfo, NULL, pRenderPass);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_VulkanRenderPasses(ocGraphicsContext* pGraphics)
{
    ocAssert(pGraphics != NULL);

    // Final composition render passes. There is one of these for image output and another for window output.
    ocResult result = ocGraphicsInit_VulkanRenderPass_FinalComposite(pGraphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &pGraphics->renderPass_FinalComposite_Image);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_VulkanRenderPass_FinalComposite(pGraphics, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &pGraphics->renderPass_FinalComposite_Window);
    if (result != OC_SUCCESS) {
        return result;
    }



    VkAttachmentDescription attachments[2];
    attachments[0].flags = 0;
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = pGraphics->msaaSamples;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;      // <-- The color image will be used as the source of a transfer to the swapchain at the end of the frame.

    attachments[1] = attachments[0];
    attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentReferences[1];
    colorAttachmentReferences[0].attachment = 0;
    colorAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthStencilAttachmentReference;
    depthStencilAttachmentReference.attachment = 1;
    depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = NULL;
    subpasses[0].colorAttachmentCount = sizeof(colorAttachmentReferences) / sizeof(colorAttachmentReferences[0]);
    subpasses[0].pColorAttachments = colorAttachmentReferences;
    subpasses[0].pResolveAttachments = NULL;
    subpasses[0].pDepthStencilAttachment = &depthStencilAttachmentReference;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = NULL;

    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = NULL;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = sizeof(subpasses) / sizeof(subpasses[0]);
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;
    VkResult vkresult = vkCreateRenderPass(pGraphics->device, &renderPassInfo, NULL, &pGraphics->renderPass0);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_VulkanPipelines(ocGraphicsContext* pGraphics)
{
    VkResult vkresult = VK_SUCCESS;

    // Shaders.
    vkresult = ocvkCreateShaderModule(pGraphics->device, sizeof(g_ocShader_Default_VERTEX), (const uint32_t*)g_ocShader_Default_VERTEX, NULL, &pGraphics->mainPipeline_VS);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    vkresult = ocvkCreateShaderModule(pGraphics->device, sizeof(g_ocShader_Default_FRAGMENT), (const uint32_t*)g_ocShader_Default_FRAGMENT, NULL, &pGraphics->mainPipeline_FS);
    if (vkresult != VK_SUCCESS) {
        vkDestroyShaderModule(pGraphics->device, pGraphics->mainPipeline_VS, NULL);
        return ocToResultFromVulkan(vkresult);
    }
    
    VkPipelineShaderStageCreateInfo pStages[2];
    pStages[0] = ocvkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, pGraphics->mainPipeline_VS, "main", NULL);
    pStages[1] = ocvkPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, pGraphics->mainPipeline_FS, "main", NULL);


    // Vertex Input.
    VkVertexInputBindingDescription pVertexBindingDescriptions[1];
    pVertexBindingDescriptions[0].binding = 0;
    pVertexBindingDescriptions[0].stride = sizeof(float) * (3+2+3);
    pVertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription pVertexAttributeDescriptions[3];
    pVertexAttributeDescriptions[0].location = 0;
    pVertexAttributeDescriptions[0].binding = 0;
    pVertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    pVertexAttributeDescriptions[0].offset = 0;
    pVertexAttributeDescriptions[1].location = 1;
    pVertexAttributeDescriptions[1].binding = 0;
    pVertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    pVertexAttributeDescriptions[1].offset = sizeof(float) * (3);
    pVertexAttributeDescriptions[2].location = 2;
    pVertexAttributeDescriptions[2].binding = 0;
    pVertexAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    pVertexAttributeDescriptions[2].offset = sizeof(float) * (3+2);

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;
    vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.pNext = NULL;
    vertexInputStateInfo.flags = 0;
    vertexInputStateInfo.vertexBindingDescriptionCount = ocCountOf(pVertexBindingDescriptions);
    vertexInputStateInfo.pVertexBindingDescriptions = pVertexBindingDescriptions;
    vertexInputStateInfo.vertexAttributeDescriptionCount = ocCountOf(pVertexAttributeDescriptions);
    vertexInputStateInfo.pVertexAttributeDescriptions = pVertexAttributeDescriptions;


    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.pNext = NULL;
    inputAssemblyStateInfo.flags = 0;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;


    // Viewport state.
    VkPipelineViewportStateCreateInfo viewportStateInfo;
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.pNext = NULL;
    viewportStateInfo.flags = 0;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = NULL;    // <-- This pipeline uses dynamic viewports.
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = NULL;     // <-- This pipeline uses dynamic scissor rectangles.


    // Rasterization state.
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo;
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.pNext = NULL;
    rasterizationStateInfo.flags = 0;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;      // <-- TODO: Research this. Don't know what this one means...
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateInfo.depthBiasConstantFactor = 1;
    rasterizationStateInfo.depthBiasClamp = 0;
    rasterizationStateInfo.depthBiasSlopeFactor = 1;
    rasterizationStateInfo.lineWidth = 1;


    // Multisample state.
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo;
    multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.pNext = NULL;
    multisampleStateInfo.flags = 0;
    multisampleStateInfo.rasterizationSamples = pGraphics->msaaSamples;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;    // TODO: Research these sampling properties.
    multisampleStateInfo.minSampleShading = VK_FALSE;
    multisampleStateInfo.pSampleMask = NULL;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;


    // Depth/Stencil state.
    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;
    memset(&depthStencilStateInfo, 0, sizeof(depthStencilStateInfo));
    depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateInfo.pNext = NULL;
    depthStencilStateInfo.flags = 0;
    depthStencilStateInfo.depthTestEnable = VK_FALSE;       // TODO: Change this to true. Just setting to false for now to try and get something showing on the screen ASAP.
    depthStencilStateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateInfo.front;
    depthStencilStateInfo.back;
    depthStencilStateInfo.minDepthBounds = 0;
    depthStencilStateInfo.maxDepthBounds = 1;


    // Color blend state.
    VkPipelineColorBlendAttachmentState pColorBlendStateAttachments[1];
    memset(pColorBlendStateAttachments, 0, sizeof(pColorBlendStateAttachments));
    pColorBlendStateAttachments[0].blendEnable = VK_FALSE;
    pColorBlendStateAttachments[0].srcColorBlendFactor;
    pColorBlendStateAttachments[0].dstColorBlendFactor;
    pColorBlendStateAttachments[0].colorBlendOp;
    pColorBlendStateAttachments[0].srcAlphaBlendFactor;
    pColorBlendStateAttachments[0].dstAlphaBlendFactor;
    pColorBlendStateAttachments[0].alphaBlendOp;
    pColorBlendStateAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
    memset(&colorBlendStateInfo, 0, sizeof(colorBlendStateInfo));
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.pNext = NULL;
    colorBlendStateInfo.flags = 0;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_CLEAR;
    colorBlendStateInfo.attachmentCount = ocCountOf(pColorBlendStateAttachments);
    colorBlendStateInfo.pAttachments = pColorBlendStateAttachments;
    //colorBlendStateInfo.blendConstants;


    // Dynamic state.
    VkDynamicState pDynamicStates[2];
    pDynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    pDynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.pNext = NULL;
    dynamicStateInfo.flags = 0;
    dynamicStateInfo.dynamicStateCount = ocCountOf(pDynamicStates);
    dynamicStateInfo.pDynamicStates = pDynamicStates;


    // Layout.
    // TODO: Move the descriptor set creation stuff to the top with the shaders. Can also move this out of the pipeline creation routine.
    VkDescriptorSetLayoutBinding pBindings[3];
    pBindings[0].binding = 0;
    pBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pBindings[0].descriptorCount = 1;
    pBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pBindings[0].pImmutableSamplers = NULL;
    pBindings[1].binding = 1;
    pBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pBindings[1].descriptorCount = 1;
    pBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pBindings[1].pImmutableSamplers = NULL;
    pBindings[2].binding = 2;
    pBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pBindings[2].descriptorCount = 1;
    pBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pBindings[2].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo;
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pNext = NULL;
    setLayoutInfo.flags = 0;
    setLayoutInfo.bindingCount = ocCountOf(pBindings);
    setLayoutInfo.pBindings = pBindings;
    vkresult = vkCreateDescriptorSetLayout(pGraphics->device, &setLayoutInfo, NULL, &pGraphics->mainPipeline_DescriptorSetLayouts[0]);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    // TODO: Move the pipeline creation info to the top.
    VkPipelineLayoutCreateInfo layoutInfo;
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = NULL;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = ocCountOf(pGraphics->mainPipeline_DescriptorSetLayouts);
    layoutInfo.pSetLayouts = pGraphics->mainPipeline_DescriptorSetLayouts;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = NULL;
    vkresult = vkCreatePipelineLayout(pGraphics->device, &layoutInfo, NULL, &pGraphics->mainPipeline_Layout);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }


    // Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = NULL;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = ocCountOf(pStages);
    pipelineInfo.pStages = pStages;
    pipelineInfo.pVertexInputState = &vertexInputStateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    pipelineInfo.pTessellationState = NULL; // <-- No tessellation at the moment.
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateInfo;
    pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = pGraphics->mainPipeline_Layout;
    pipelineInfo.renderPass = pGraphics->renderPass0;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = NULL;
    pipelineInfo.basePipelineIndex = 0;
    vkresult = vkCreateGraphicsPipelines(pGraphics->device, NULL, 1, &pipelineInfo, NULL, &pGraphics->mainPipeline);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_VulkanSamplers(ocGraphicsContext* pGraphics)
{
    ocAssert(pGraphics != NULL);

    VkSamplerCreateInfo samplerInfo;
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = NULL;
    samplerInfo.flags = 0;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = 32;    // <-- I've just set this to something generic and high enough to work with every image. Is this OK, or is it better to use image-specific samplers where this is set to the number of mipmaps?
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    VkResult vkresult = vkCreateSampler(pGraphics->device, &samplerInfo, NULL, &pGraphics->sampler_Linear);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    vkresult = vkCreateSampler(pGraphics->device, &samplerInfo, NULL, &pGraphics->sampler_Nearest);
    if (vkresult != VK_SUCCESS) {
        vkDestroySampler(pGraphics->device, pGraphics->sampler_Linear, NULL);
        return ocToResultFromVulkan(vkresult);
    }

    return OC_SUCCESS;
}

OC_PRIVATE ocResult ocGraphicsInit_Vulkan(ocGraphicsContext* pGraphics, uint32_t desiredMSAASamples)
{
    ocResult result = ocGraphicsInit_VulkanInstance(pGraphics);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_VulkanDevices(pGraphics, desiredMSAASamples);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_VulkanRenderPasses(pGraphics);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_VulkanPipelines(pGraphics);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_VulkanSamplers(pGraphics);
    if (result != OC_SUCCESS) {
        return result;
    }


    // TODO: Re-asses how we manage descriptor pools. Currently thinking we have one pool for known pipelines like the final composition and
    // then a bunch of other pools for different material types.
    VkDescriptorPoolSize pPoolSizes[2];
    pPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pPoolSizes[0].descriptorCount = 1024;
    pPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pPoolSizes[1].descriptorCount = 1024;

    VkDescriptorPoolCreateInfo descriptorPoolInfo;
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = NULL;
    descriptorPoolInfo.flags = 0;
    descriptorPoolInfo.maxSets = 1024;
    descriptorPoolInfo.poolSizeCount = ocCountOf(pPoolSizes);
    descriptorPoolInfo.pPoolSizes = pPoolSizes;
    VkResult vkresult = vkCreateDescriptorPool(pGraphics->device, &descriptorPoolInfo, NULL, &pGraphics->descriptorPool);
    if (vkresult != NULL) {
        return ocToResultFromVulkan(vkresult);
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo;
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.pNext = NULL;
    descriptorSetAllocInfo.descriptorPool = pGraphics->descriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    descriptorSetAllocInfo.pSetLayouts = pGraphics->mainPipeline_DescriptorSetLayouts;
    vkresult = vkAllocateDescriptorSets(pGraphics->device, &descriptorSetAllocInfo, &pGraphics->mainPipeline_DescriptorSets[0]);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    return OC_SUCCESS;
}

ocResult ocGraphicsInit(ocEngineContext* pEngine, uint32_t desiredMSAASamples, ocGraphicsContext* pGraphics)
{
    ocResult result = ocGraphicsInitBase(pEngine, pGraphics);
    if (result != OC_SUCCESS) {
        return result;
    }

    result = ocGraphicsInit_Vulkan(pGraphics, desiredMSAASamples);
    if (result != OC_SUCCESS) {
        return result;
    }


    // Feature support.

    // Vulkan always supports at least 4x MSAA.
    pGraphics->supportFlags |= OC_GRAPHICS_SUPPORT_FLAG_MSAA;

    // Assume support for adaptive vsync for now. It's actually per-swapchain.
    pGraphics->supportFlags |= OC_GRAPHICS_SUPPORT_FLAG_ADAPTIVE_VSYNC;


    return OC_SUCCESS;
}


OC_PRIVATE void ocGraphicsUninit_Vulkan(ocGraphicsContext* pGraphics)
{
    ocAssert(pGraphics != NULL);

    // TODO: Implement this fully.

    vkDestroyInstance(pGraphics->instance, NULL);
    vkbUninit();
}

void ocGraphicsUninit(ocGraphicsContext* pGraphics)
{
    if (pGraphics == NULL) return;

    ocGraphicsUninit_Vulkan(pGraphics);
    ocGraphicsUninitBase(pGraphics);
}


ocResult ocGraphicsCreateSwapchain(ocGraphicsContext* pGraphics, ocWindow* pWindow, ocVSyncMode vsyncMode, ocGraphicsSwapchain** ppSwapchain)
{
    if (ppSwapchain == NULL) return OC_INVALID_ARGS;
    *ppSwapchain = NULL;

    ocGraphicsSwapchain* pSwapchain = ocCallocObject(ocGraphicsSwapchain);
    if (pSwapchain == NULL) {
        return OC_OUT_OF_MEMORY;
    }

    ocResult result = ocGraphicsSwapchainBaseInit(pGraphics, pWindow, vsyncMode, pSwapchain);
    if (result != OC_SUCCESS) {
        ocFree(pSwapchain);
        return result;
    }


    VkResult vkresult = VK_SUCCESS;

    // We need a surface before we can create a swapchain.
#ifdef OC_WIN32
    vkresult = ocvkCreateWin32Surface(pGraphics->instance, pWindow->hWnd, NULL, &pSwapchain->vkSurface);
    if (vkresult != VK_SUCCESS) {
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }
#endif

    // We need to keep track of the size of the swapchain images.
    unsigned int sizeX;
    unsigned int sizeY;
    ocWindowGetSize(pWindow, &sizeX, &sizeY);
    pSwapchain->sizeX = ocMax(1, (uint32_t)sizeX);
    pSwapchain->sizeY = ocMax(1, (uint32_t)sizeY);


    VkBool32 isSurfaceSupported;
    vkresult = vkGetPhysicalDeviceSurfaceSupportKHR(pGraphics->physicalDevice, pGraphics->queueFamilyIndex, pSwapchain->vkSurface, &isSurfaceSupported);
    if (vkresult != VK_SUCCESS || isSurfaceSupported == VK_FALSE) {
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    // Check how many images the surface supportes before trying to create the swapchain.
    //
    // Can we relax this and simply clamp the image count and return the actual image count as an output (change imageCount to an in/out pointer)?
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkresult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pGraphics->physicalDevice, pSwapchain->vkSurface, &surfaceCaps);
    if (vkresult != VK_SUCCESS) {
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    uint32_t imageCount = 2;
    if (imageCount < surfaceCaps.minImageCount || imageCount > surfaceCaps.maxImageCount) {
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    VkFormat pDesiredFormats[] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        //VK_FORMAT_R8G8B8A8_SRGB,
        //VK_FORMAT_B8G8R8A8_SRGB,
    };

    VkSurfaceFormatKHR format = ocvkChooseSurfaceImageFormat(pGraphics->physicalDevice, pSwapchain->vkSurface, sizeof(pDesiredFormats) / sizeof(pDesiredFormats[0]), pDesiredFormats);

    // Create the swapchain after support validation.
    // TODO: Check if we can remove VK_IMAGE_USAGE_TRANSFER_DST_BIT
    vkresult = ocvkCreateSwapchain(pGraphics->physicalDevice, pGraphics->device, pSwapchain->vkSurface, imageCount, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        ocGraphicsGetBestPresentMode(pGraphics->physicalDevice, pSwapchain->vkSurface, ocVSyncMode_Adaptive), NULL, &pSwapchain->vkSwapchain);
    if (vkresult != VK_SUCCESS) {
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }


    // We need a sempahore to wait on for when we need to output to final result to the next image in the swapchain. This is signaled
    // in vkAcquireNextImageKHR() and waited on by the queue when the image is transitioned to it's new state in preparation for output.
    vkresult = ocvkCreateSemaphore(pGraphics->device, &pSwapchain->vkNextImageSem);
    if (vkresult != VK_SUCCESS) {
        vkDestroySwapchainKHR(pGraphics->device, pSwapchain->vkSwapchain, NULL);
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }


    // The images in the swapchain need to be transitioned to a known state.
    pSwapchain->vkSwapchainImageCount = 3;
    vkresult = vkGetSwapchainImagesKHR(pGraphics->device, pSwapchain->vkSwapchain, &pSwapchain->vkSwapchainImageCount, pSwapchain->vkSwapchainImages);
    if (result != VK_SUCCESS) {
        vkDestroySemaphore(pGraphics->device, pSwapchain->vkNextImageSem, NULL);
        vkDestroySwapchainKHR(pGraphics->device, pSwapchain->vkSwapchain, NULL);
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    // TODO: Optimize this. No need to clear.
    VkCommandBuffer cmdbuffer;
    VkCommandBufferAllocateInfo cmdbufferInfo;
    cmdbufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufferInfo.pNext = NULL;
    cmdbufferInfo.commandPool = pGraphics->commandPool;
    cmdbufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdbufferInfo.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(pGraphics->device, &cmdbufferInfo, &cmdbuffer);
    if (result != VK_SUCCESS) {
        vkDestroySemaphore(pGraphics->device, pSwapchain->vkNextImageSem, NULL);
        vkDestroySwapchainKHR(pGraphics->device, pSwapchain->vkSwapchain, NULL);
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = NULL;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = NULL;
    vkBeginCommandBuffer(cmdbuffer, &beginInfo);
    {
        for (uint32_t iImage = 0; iImage < imageCount; ++iImage) {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = NULL;
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = pGraphics->queueFamilyIndex;
            barrier.dstQueueFamilyIndex = pGraphics->queueFamilyIndex;
            barrier.image = pSwapchain->vkSwapchainImages[iImage];
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
            {
                // TODO: No need to do this clear. Just here for testing for the moment.
                VkClearColorValue clearColor;
                clearColor.float32[0] = 1;
                clearColor.float32[1] = 0;
                clearColor.float32[2] = 0;
                clearColor.float32[3] = 1;
                vkCmdClearColorImage(cmdbuffer, pSwapchain->vkSwapchainImages[iImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &barrier.subresourceRange);
            }
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        }
    }
    vkEndCommandBuffer(cmdbuffer);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(pGraphics->queue, 1, &submitInfo, 0);
    vkQueueWaitIdle(pGraphics->queue);
    vkFreeCommandBuffers(pGraphics->device, pGraphics->commandPool, 1, &cmdbuffer);


    // We need to acquire an initial image.
    vkresult = vkAcquireNextImageKHR(pGraphics->device, pSwapchain->vkSwapchain, UINT64_MAX, pSwapchain->vkNextImageSem, VK_NULL_HANDLE, &pSwapchain->vkCurrentImageIndex);
    if (vkresult != VK_SUCCESS && vkresult != VK_SUBOPTIMAL_KHR) {
        vkDestroySemaphore(pGraphics->device, pSwapchain->vkNextImageSem, NULL);
        vkDestroySwapchainKHR(pGraphics->device, pSwapchain->vkSwapchain, NULL);
        vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);
        ocFree(pSwapchain);
        return ocToResultFromVulkan(vkresult);
    }

    *ppSwapchain = pSwapchain;
    return OC_SUCCESS;
}

void ocGraphicsDeleteSwapchain(ocGraphicsContext* pGraphics, ocGraphicsSwapchain* pSwapchain)
{
    if (pGraphics == NULL || pSwapchain == NULL) return;

    // Before doing anything, make sure the device isn't using the swapchain.
    vkDeviceWaitIdle(pGraphics->device);

    vkDestroySemaphore(pGraphics->device, pSwapchain->vkNextImageSem, NULL);
    vkDestroySwapchainKHR(pGraphics->device, pSwapchain->vkSwapchain, NULL);
    vkDestroySurfaceKHR(pGraphics->instance, pSwapchain->vkSurface, NULL);

    ocGraphicsSwapchainBaseUninit(pSwapchain);
}

void ocGraphicsGetSwapchainSize(ocGraphicsContext* pGraphics, ocGraphicsSwapchain* pSwapchain, uint32_t* pSizeX, uint32_t* pSizeY)
{
    if (pSizeX != NULL) *pSizeX = 0;
    if (pSizeY != NULL) *pSizeY = 0;
    if (pGraphics == NULL || pSwapchain == NULL) return;

    if (pSizeX != NULL) *pSizeX = pSwapchain->sizeX;
    if (pSizeY != NULL) *pSizeY = pSwapchain->sizeY;
}

void ocGraphicsPresent(ocGraphicsContext* pGraphics, ocGraphicsSwapchain* pSwapchain)
{
    if (pGraphics == NULL || pSwapchain == NULL) return;

    VkPresentInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext = NULL;
    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = NULL;
    info.swapchainCount = 1;
    info.pSwapchains = &pSwapchain->vkSwapchain;
    info.pImageIndices = &pSwapchain->vkCurrentImageIndex;
    info.pResults = NULL;
    VkResult vkresult = vkQueuePresentKHR(pGraphics->queue, &info);
    if (vkresult != VK_SUCCESS) {
        return;
    }

    vkresult = vkAcquireNextImageKHR(pGraphics->device, pSwapchain->vkSwapchain, UINT64_MAX, pSwapchain->vkNextImageSem, VK_NULL_HANDLE, &pSwapchain->vkCurrentImageIndex);
    if (vkresult != VK_SUCCESS && vkresult != VK_SUBOPTIMAL_KHR) {
        if (vkresult == VK_ERROR_OUT_OF_DATE_KHR) {
            // Re-create the swapchain?
            // TODO: HANDLE ME PROPERLY.
        }

        return;
    }
}


ocResult ocGraphicsCreateImage(ocGraphicsContext* pGraphics, ocGraphicsImageDesc* pDesc, ocGraphicsImage** ppImage)
{
    if (ppImage == NULL) return OC_INVALID_ARGS;
    *ppImage = NULL;

    if (pGraphics == NULL || pDesc == NULL) return OC_INVALID_ARGS;
    if (pDesc->mipLevels == 0 || pDesc->usage == 0) return OC_INVALID_ARGS;

    ocGraphicsImage* pImage = ocMallocObject(ocGraphicsImage);
    if (pImage == NULL) {
        return OC_OUT_OF_MEMORY;
    }

    pImage->format = ocToVulkanImageFormat(pDesc->format);
    pImage->usage = 0;
    if (ocIsBitSet(pDesc->usage, OC_GRAPHICS_IMAGE_USAGE_SHADER_INPUT)) {
        pImage->usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (ocIsBitSet(pDesc->usage, OC_GRAPHICS_IMAGE_USAGE_RENDER_TARGET)) {
        pImage->usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    if (pDesc->pImageData != NULL) {
        pImage->usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    pImage->sizeX = pDesc->pMipmaps[0].width;
    pImage->sizeY = pDesc->pMipmaps[0].height;
    pImage->mipLevels = pDesc->mipLevels;

    VkImageCreateInfo imageInfo;
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = NULL;
    imageInfo.flags = 0;;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = pImage->format;
    imageInfo.extent = ocvkExtent3D(pImage->sizeX, pImage->sizeY, 1);
    imageInfo.mipLevels = pDesc->mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = pImage->usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = NULL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult vkresult = vkCreateImage(pGraphics->device, &imageInfo, NULL, &pImage->imageVK);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    vkresult = ocvkAllocateAndBindImageMemory(pGraphics->physicalDevice, pGraphics->device, pImage->imageVK, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NULL, &pImage->imageMemoryVK);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }


    // If we have a pointer to some initial data we'll want to upload that.
    if (pDesc->pImageData != NULL) {
        // We need a staging buffer for the image data. The data for each mipmap is tighly packed into the main buffer.
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vkresult = ocvkCreateStagingBuffer(pGraphics->physicalDevice, pGraphics->device, pDesc->imageDataSize, pDesc->pImageData, NULL, &stagingBuffer, &stagingBufferMemory);
        if (vkresult != VK_SUCCESS) {
            vkDestroyImage(pGraphics->device, pImage->imageVK, NULL);
            vkFreeMemory(pGraphics->device, pImage->imageMemoryVK, NULL);
            return ocToResultFromVulkan(vkresult);
        }

        // Finally we need to copy the data from the staging buffer over to the image. We need a command buffer for this.
        VkBufferImageCopy regions[32];
        for (uint32_t iMipmap = 0; iMipmap < pImage->mipLevels; ++iMipmap) {
            regions[iMipmap].bufferOffset = pDesc->pMipmaps[iMipmap].dataOffset;
            regions[iMipmap].bufferRowLength = 0;
            regions[iMipmap].bufferImageHeight = 0;
            regions[iMipmap].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[iMipmap].imageSubresource.mipLevel = iMipmap;
            regions[iMipmap].imageSubresource.baseArrayLayer = 0;
            regions[iMipmap].imageSubresource.layerCount = 1;
            regions[iMipmap].imageOffset.x = 0;
            regions[iMipmap].imageOffset.y = 0;
            regions[iMipmap].imageOffset.z = 0;
            regions[iMipmap].imageExtent.width  = pDesc->pMipmaps[iMipmap].width;
            regions[iMipmap].imageExtent.height = pDesc->pMipmaps[iMipmap].height;
        }

        VkCommandBuffer cmdbuffer;
        vkresult = ocvkAllocateCommandBuffers(pGraphics->device, pGraphics->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &cmdbuffer);
        if (vkresult != VK_SUCCESS) {
            vkDestroyImage(pGraphics->device, pImage->imageVK, NULL);
            vkFreeMemory(pGraphics->device, pImage->imageMemoryVK, NULL);
            vkDestroyBuffer(pGraphics->device, stagingBuffer, NULL);
            vkFreeMemory(pGraphics->device, stagingBufferMemory, NULL);
            return ocToResultFromVulkan(vkresult);
        }

        ocvkBeginCommandBuffer(cmdbuffer, 0, NULL);
        {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = NULL;
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = pGraphics->queueFamilyIndex;
            barrier.dstQueueFamilyIndex = pGraphics->queueFamilyIndex;
            barrier.image = pImage->imageVK;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = pDesc->mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
            {
                vkCmdCopyBufferToImage(cmdbuffer, stagingBuffer, pImage->imageVK, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pDesc->mipLevels, regions);
            }
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = ocIsBitSet(pDesc->usage, OC_GRAPHICS_IMAGE_USAGE_SHADER_INPUT) ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_MEMORY_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = ocIsBitSet(pDesc->usage, OC_GRAPHICS_IMAGE_USAGE_SHADER_INPUT) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        }
        vkEndCommandBuffer(cmdbuffer);

        
        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = NULL;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = NULL;
        submitInfo.pWaitDstStageMask = NULL;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdbuffer;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;
        vkQueueSubmit(pGraphics->queue, 1, &submitInfo, 0);
        vkQueueWaitIdle(pGraphics->queue);
        vkFreeCommandBuffers(pGraphics->device, pGraphics->commandPool, 1, &cmdbuffer);

        vkFreeMemory(pGraphics->device, stagingBufferMemory, NULL);
        vkDestroyBuffer(pGraphics->device, stagingBuffer, NULL);
    } else {
        // There was no image data so we just want to transition the images to their initial state.
        // TODO: Implement me.
    }


    // The image view. This is always the same format as the main image.
    vkresult = ocvkCreateSimpleImageView(pGraphics->device, pImage->imageVK, VK_IMAGE_VIEW_TYPE_2D, pImage->format, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, pDesc->mipLevels, 0, 1), NULL, &pImage->imageViewVK);
    if (vkresult != VK_SUCCESS) {
        // TODO: Cleanup.
        return ocToResultFromVulkan(vkresult);
    }


    pImage->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    pImage->descriptor.imageView = pImage->imageViewVK;
    pImage->descriptor.sampler = pGraphics->sampler_Nearest;

    *ppImage = pImage;
    return OC_SUCCESS;
}

void ocGraphicsDeleteImage(ocGraphicsContext* pGraphics, ocGraphicsImage* pImage)
{
    if (pGraphics == NULL || pImage == NULL) return;

    vkFreeMemory(pGraphics->device, pImage->imageVK, NULL);
    vkDestroyImage(pGraphics->device, pImage->imageVK, NULL);
}


ocResult ocGraphicsCreateMesh(ocGraphicsContext* pGraphics, ocGraphicsMeshDesc* pDesc, ocGraphicsMesh** ppMesh)
{
    if (ppMesh == NULL) return OC_INVALID_ARGS;
    *ppMesh = NULL;

    if (pGraphics == NULL || pDesc == NULL) return OC_INVALID_ARGS;

    ocGraphicsMesh* pMesh = ocCallocObject(ocGraphicsMesh);
    if (pMesh == NULL) {
        return OC_OUT_OF_MEMORY;
    }

    pMesh->primitiveType = pDesc->primitiveType;
    pMesh->vertexFormat = pDesc->vertexFormat;
    pMesh->indexFormat = pDesc->indexFormat;
    pMesh->indexCount = pDesc->indexCount;

    size_t vertexBufferSize = pDesc->vertexCount * ocGetVertexSizeFromFormat(pDesc->vertexFormat);
    size_t indexBufferSize = pDesc->indexCount * ocGetIndexSizeFromFormat(pDesc->indexFormat);


    // Buffer objects.
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = NULL;
    bufferInfo.flags = 0;
    bufferInfo.size = vertexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = NULL;
    VkResult vkresult = vkCreateBuffer(pGraphics->device, &bufferInfo, NULL, &pMesh->vertexBufferVK);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    bufferInfo.size = indexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkresult = vkCreateBuffer(pGraphics->device, &bufferInfo, NULL, &pMesh->indexBufferVK);
    if (vkresult != VK_SUCCESS) {
        vkDestroyBuffer(pGraphics->device, pMesh->vertexBufferVK, NULL);
        return ocToResultFromVulkan(vkresult);
    }


    // Memory.
    //
    // The memory for buffers is always device local for the sake of performance. To fill this with data we need to use a staging buffer.
    //
    // TODO: Make this device local and use a staging buffer.

    vkresult = ocvkAllocateAndBindBufferMemory(pGraphics->physicalDevice, pGraphics->device, pMesh->vertexBufferVK, /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, NULL, &pMesh->vertexBufferMemory);
    if (vkresult != VK_SUCCESS) {
        vkDestroyBuffer(pGraphics->device, pMesh->vertexBufferVK, NULL);
        vkDestroyBuffer(pGraphics->device, pMesh->indexBufferVK, NULL);
        return ocToResultFromVulkan(vkresult);
    }

    void* pVertexData;
    vkresult = vkMapMemory(pGraphics->device, pMesh->vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &pVertexData);
    ocCopyMemory(pVertexData, pDesc->pVertices, vertexBufferSize);
    vkUnmapMemory(pGraphics->device, pMesh->vertexBufferMemory);


    vkresult = ocvkAllocateAndBindBufferMemory(pGraphics->physicalDevice, pGraphics->device, pMesh->indexBufferVK, /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, NULL, &pMesh->indexBufferMemory);
    if (vkresult != VK_SUCCESS) {
        vkDestroyBuffer(pGraphics->device, pMesh->vertexBufferVK, NULL);
        vkDestroyBuffer(pGraphics->device, pMesh->indexBufferVK, NULL);
        vkFreeMemory(pGraphics->device, pMesh->vertexBufferMemory, NULL);
        return ocToResultFromVulkan(vkresult);
    }

    void* pIndexData;
    vkresult = vkMapMemory(pGraphics->device, pMesh->indexBufferMemory, 0, VK_WHOLE_SIZE, 0, &pIndexData);
    ocCopyMemory(pIndexData, pDesc->pIndices, indexBufferSize);
    vkUnmapMemory(pGraphics->device, pMesh->indexBufferMemory);



    *ppMesh = pMesh;
    return OC_SUCCESS;
}

void ocGraphicsDeleteMesh(ocGraphicsContext* pGraphics, ocGraphicsMesh* pMesh)
{
    if (pGraphics == NULL || pMesh == NULL) return;
    ocFree(pMesh);
}



///////////////////////////////////////////////////////////////////////////////
//
// GraphicsWorld
//
///////////////////////////////////////////////////////////////////////////////

ocResult ocGraphicsWorldInit(ocGraphicsContext* pGraphics, ocGraphicsWorld* pWorld)
{
    ocResult result = ocGraphicsWorldInitBase(pGraphics, pWorld);
    if (result != OC_SUCCESS) {
        return result;
    }

    pWorld->pObjects = new std::vector<ocGraphicsObject*>();

    return OC_SUCCESS;
}

void ocGraphicsWorldUninit(ocGraphicsWorld* pWorld)
{
    if (pWorld == NULL) return;

    delete pWorld->pObjects;
    ocGraphicsWorldUninitBase(pWorld);
}


void ocGraphicsWorldDraw(ocGraphicsWorld* pWorld)
{
    if (pWorld == NULL) return;

    for (uint16_t iRT = 0; iRT < pWorld->renderTargetCount; ++iRT) {
        ocGraphicsWorldDrawRT(pWorld, pWorld->pRenderTargets[iRT]);
    }
}

void ocGraphicsWorldDrawRT(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    if (pWorld == NULL || pRT == NULL) return;

    // Update the uniform buffers for the render target.
    glm::mat4 projection = pRT->projection * ocMakeMat4_VulkanClipCorrection();
    glm::mat4 view       = pRT->view;

    memcpy(ocOffsetPtr(pRT->pUniformBufferData, 0),                 &projection, sizeof(pRT->projection));
    memcpy(ocOffsetPtr(pRT->pUniformBufferData, sizeof(glm::mat4)), &view,       sizeof(pRT->view));

    // The descriptor set for the RTs uniforms need to be updated.
    VkWriteDescriptorSet descriptorWrite;
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = NULL;
    descriptorWrite.dstSet = pWorld->pGraphics->mainPipeline_DescriptorSets[0];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pImageInfo = NULL;
    descriptorWrite.pBufferInfo = &pRT->uniformBufferDescriptor;
    descriptorWrite.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(pWorld->pGraphics->device, 1, &descriptorWrite, 0, NULL);

    descriptorWrite.dstSet = pWorld->pGraphics->mainPipeline_DescriptorSets[0];
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pImageInfo = NULL;
    descriptorWrite.pBufferInfo = &pWorld->pObjects->at(0)->uniformBufferDescriptor;
    descriptorWrite.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(pWorld->pGraphics->device, 1, &descriptorWrite, 0, NULL);

    descriptorWrite.dstSet = pWorld->pGraphics->mainPipeline_DescriptorSets[0];
    descriptorWrite.dstBinding = 2;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &pWorld->pCurrentImage->descriptor;
    descriptorWrite.pBufferInfo = NULL;
    descriptorWrite.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(pWorld->pGraphics->device, 1, &descriptorWrite, 0, NULL);




    // Steps:
    // 1) Transition render targets.

    // The first thing we always do is transition resources into the correct state. This is done in one go at the top and queued
    // as soon as possible. While that's running we build the command queues for the next group of operations.

    // TODO:
    // - Use pre-recorded command buffers.

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pRT->cbPreTransition;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(pWorld->pGraphics->queue, 1, &submitInfo, 0);


    // Transition the swapchain image to a write state.
    if (pRT->pSwapchain != NULL) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &pRT->pSwapchain->vkNextImageSem;
        submitInfo.pWaitDstStageMask = NULL;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &pRT->cbPreTransition_Outputs[pRT->pSwapchain->vkCurrentImageIndex];
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = NULL;
        vkQueueSubmit(pWorld->pGraphics->queue, 1, &submitInfo, 0);
    }


    // RENDER STUFF

    VkCommandBuffer cmdbuf;
    VkResult vkresult = ocvkAllocateCommandBuffers(pWorld->pGraphics->device, pWorld->pGraphics->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &cmdbuf);
    if (vkresult != VK_SUCCESS) {
        return;
    }

    ocvkBeginCommandBuffer(cmdbuf, 0, NULL);
    {
        VkClearValue pClearValues[2];
        pClearValues[0] = ocvkClearValueColor4f(0, 0, 1, 1);
        pClearValues[1] = ocvkClearValueDepthStencil(1.0, 0);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = NULL;
        renderPassBeginInfo.renderPass = pWorld->pGraphics->renderPass0;
        renderPassBeginInfo.framebuffer = pRT->mainFramebuffer;
        renderPassBeginInfo.renderArea = ocvkRect2D(0, 0, pRT->sizeX, pRT->sizeY);
        renderPassBeginInfo.clearValueCount = ocCountOf(pClearValues);
        renderPassBeginInfo.pClearValues = pClearValues;
        vkCmdBeginRenderPass(cmdbuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            VkViewport viewport = ocvkViewport(0, 0, (float)pRT->sizeX, (float)pRT->sizeY, 0, 1);
            vkCmdSetViewport(cmdbuf, 0, 1, &viewport);

            VkRect2D scissor = ocvkRect2D(0, 0, pRT->sizeX, pRT->sizeY);
            vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

            // Draw some objects.
            for (size_t iObject = 0; iObject < pWorld->pObjects->size(); ++iObject) {
                ocGraphicsObject* pObject = pWorld->pObjects->at(iObject);
                if (pObject->type == ocGraphicsObjectType_Mesh) {
                    ocGraphicsMesh* pMesh = pObject->data.mesh.pResource;   // <-- For ease of use.

                    vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pWorld->pGraphics->mainPipeline);
                    vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pWorld->pGraphics->mainPipeline_Layout, 0, 1, &pWorld->pGraphics->mainPipeline_DescriptorSets[0], 0, NULL);

                    vkCmdBindVertexBuffers(cmdbuf, 0, 1, &pMesh->vertexBufferVK, &pMesh->vertexBufferOffset);
                    vkCmdBindIndexBuffer(cmdbuf, pMesh->indexBufferVK, pMesh->indexBufferOffset, ocToVulkanIndexFormat(pMesh->indexFormat));
                    vkCmdDrawIndexed(cmdbuf, pMesh->indexCount, 1, 0, 0, 0);
                }
            }
        }
        vkCmdEndRenderPass(cmdbuf);

        // Final Composition
        //
        // The final composition is done differently depending on whether or not we are outputting to an image or a window.
        if (pRT->pSwapchain != NULL) {
            renderPassBeginInfo.renderPass = pWorld->pGraphics->renderPass_FinalComposite_Window;
            renderPassBeginInfo.framebuffer = pRT->outputFBs[pRT->pSwapchain->vkCurrentImageIndex];
        } else {
            renderPassBeginInfo.renderPass = pWorld->pGraphics->renderPass_FinalComposite_Image;
            renderPassBeginInfo.framebuffer = pRT->outputFBs[0];
        }

        renderPassBeginInfo.renderArea = ocvkRect2D(0, 0, pRT->sizeX, pRT->sizeY);
        renderPassBeginInfo.clearValueCount = 0;
        renderPassBeginInfo.pClearValues = NULL;
        vkCmdBeginRenderPass(cmdbuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            // TODO:
            // - Set viewport
            // - Set scissor
            // - Set pipeline
            // - Draw a fullscreen quad or triangle.
        }
        vkCmdEndRenderPass(cmdbuf);
    }
    vkEndCommandBuffer(cmdbuf);

    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdbuf;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(pWorld->pGraphics->queue, 1, &submitInfo, 0);
    vkQueueWaitIdle(pWorld->pGraphics->queue);

    vkFreeCommandBuffers(pWorld->pGraphics->device, pWorld->pGraphics->commandPool, 1, &cmdbuf);
}


void ocGraphicsWorldStep(ocGraphicsWorld* pWorld, double dt)
{
    if (pWorld == NULL || dt == 0) return;
    
    // TODO: Step animations and particle effects.
}



OC_PRIVATE ocResult ocGraphicsWorldAddRT(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    if (pWorld == NULL) return OC_INVALID_ARGS;
    if (pWorld->renderTargetCount == OC_MAX_RENDER_TARGETS) return OC_TOO_MANY_RENDER_TARGETS;

    pWorld->pRenderTargets[pWorld->renderTargetCount++] = pRT;

    return OC_SUCCESS;
}

OC_PRIVATE void ocGraphicsWorldRemoveRTByIndex(ocGraphicsWorld* pWorld, uint16_t index)
{
    ocAssert(pWorld != NULL);
    if (pWorld->renderTargetCount == 0) return;

    for (uint16_t i = index; i < pWorld->renderTargetCount-1; ++i) {
        pWorld->pRenderTargets[i] = pWorld->pRenderTargets[i+1];
    }

    pWorld->renderTargetCount -= 1;
}

OC_PRIVATE void ocGraphicsWorldRemoveRT(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    if (pWorld == NULL || pRT == NULL) return;

    for (uint16_t iRT = 0; iRT < pWorld->renderTargetCount; ++iRT) {
        if (pWorld->pRenderTargets[iRT] == pRT) {
            ocGraphicsWorldRemoveRTByIndex(pWorld, iRT);
            break;
        }
    }
}


OC_PRIVATE void ocGraphicsWorldUninitRT_MainFramebuffer(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    ocAssert(pWorld != NULL);
    ocAssert(pRT != NULL);

    if (pRT->colorImageView) vkDestroyImageView(pWorld->pGraphics->device, pRT->colorImageView, NULL);
    if (pRT->colorImageMemory) vkFreeMemory(pWorld->pGraphics->device, pRT->colorImageMemory, NULL);
    if (pRT->colorImage) vkDestroyImage(pWorld->pGraphics->device, pRT->colorImage, NULL);
    if (pRT->dsImageView) vkDestroyImageView(pWorld->pGraphics->device, pRT->dsImageView, NULL);
    if (pRT->dsImageMemory) vkFreeMemory(pWorld->pGraphics->device, pRT->dsImageMemory, NULL);
    if (pRT->dsImage) vkDestroyImage(pWorld->pGraphics->device, pRT->dsImage, NULL);
    if (pRT->mainFramebuffer) vkDestroyFramebuffer(pWorld->pGraphics->device, pRT->mainFramebuffer, NULL);
}

OC_PRIVATE ocResult ocGraphicsWorldAllocAndInitRT_MainFramebuffer(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT, uint32_t sizeX, uint32_t sizeY)
{
    ocAssert(pWorld != NULL);
    ocAssert(pRT != NULL);
    ocAssert(sizeX > 0);
    ocAssert(sizeY > 0);

    // Image objects.
    VkImageCreateInfo imageInfo = ocvkSimpleImageCreateInfo2D(VK_FORMAT_R8G8B8A8_UNORM, sizeX, sizeY, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    imageInfo.samples = pWorld->pGraphics->msaaSamples;
    VkResult vkresult = vkCreateImage(pWorld->pGraphics->device, &imageInfo, NULL, &pRT->colorImage);
    if (vkresult != VK_SUCCESS) {
        return ocToResultFromVulkan(vkresult);
    }

    imageInfo = ocvkSimpleImageCreateInfo2D(VK_FORMAT_D24_UNORM_S8_UINT, sizeX, sizeY, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    imageInfo.samples = pWorld->pGraphics->msaaSamples;
    vkresult = vkCreateImage(pWorld->pGraphics->device, &imageInfo, NULL, &pRT->dsImage);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }


    // Memory
    vkresult = ocvkAllocateAndBindImageMemory(pWorld->pGraphics->physicalDevice, pWorld->pGraphics->device, pRT->colorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NULL, &pRT->colorImageMemory);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }

    vkresult = ocvkAllocateAndBindImageMemory(pWorld->pGraphics->physicalDevice, pWorld->pGraphics->device, pRT->dsImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NULL, &pRT->dsImageMemory);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }


    // Views
    vkresult = ocvkCreateSimpleImageView(pWorld->pGraphics->device, pRT->colorImage, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1), NULL, &pRT->colorImageView);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }

    vkresult = ocvkCreateSimpleImageView(pWorld->pGraphics->device, pRT->dsImage, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D24_UNORM_S8_UINT, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1), NULL, &pRT->dsImageView);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }


    // Framebuffer
    VkImageView attachments[2];
    attachments[0] = pRT->colorImageView;
    attachments[1] = pRT->dsImageView;

    VkFramebufferCreateInfo fbInfo;
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.pNext = NULL;
    fbInfo.flags = 0;
    fbInfo.renderPass = pWorld->pGraphics->renderPass0;
    fbInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    fbInfo.pAttachments = attachments;
    fbInfo.width = sizeX;
    fbInfo.height = sizeY;
    fbInfo.layers = 1;
    vkresult = vkCreateFramebuffer(pWorld->pGraphics->device, &fbInfo, NULL, &pRT->mainFramebuffer);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }


    // Transition images to a known state.
    vkresult = ocGraphicsDoInitialImageLayoutTransition(pWorld->pGraphics, pRT->colorImage, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }

    vkresult = ocGraphicsDoInitialImageLayoutTransition(pWorld->pGraphics, pRT->dsImage, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1), VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }


    // Create the command buffer that is used to transition the framebuffers into their initial state.
    vkresult = ocvkAllocateCommandBuffers(pWorld->pGraphics->device, pWorld->pGraphics->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &pRT->cbPreTransition);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        return ocToResultFromVulkan(vkresult);
    }

    ocvkBeginCommandBuffer(pRT->cbPreTransition, 0, NULL);
    {
        VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = pWorld->pGraphics->queueFamilyIndex;
        barrier.dstQueueFamilyIndex = pWorld->pGraphics->queueFamilyIndex;
        barrier.image = pRT->colorImage;
        barrier.subresourceRange = ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
        vkCmdPipelineBarrier(pRT->cbPreTransition, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    vkEndCommandBuffer(pRT->cbPreTransition);

    return OC_SUCCESS;
}


OC_PRIVATE void ocGraphicsWorldUninitRT_OutputFramebuffers(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    ocAssert(pWorld != NULL);
    ocAssert(pRT != NULL);

    for (uint32_t iOutputImage = 0; iOutputImage < pRT->outputImageCount; ++iOutputImage) {
        if (pRT->outputFBs[iOutputImage] != NULL) vkDestroyFramebuffer(pWorld->pGraphics->device, pRT->outputFBs[iOutputImage], NULL);
        if (pRT->outputImageViews[iOutputImage] != NULL) vkDestroyImageView(pWorld->pGraphics->device, pRT->outputImageViews[iOutputImage], NULL);
        if (pRT->cbPreTransition_Outputs[iOutputImage] != NULL) vkFreeCommandBuffers(pWorld->pGraphics->device, pWorld->pGraphics->commandPool, 1, &pRT->cbPreTransition_Outputs[iOutputImage]);
    }
}

OC_PRIVATE ocResult ocGraphicsWorldAllocAndInitRT_OutputFramebuffers(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT, uint32_t sizeX, uint32_t sizeY, uint32_t outputImageCount, VkImage* pOutputImages, VkFormat outputImageFormat)
{
    ocAssert(pWorld != NULL);
    ocAssert(pRT != NULL);
    ocAssert(sizeX > 0);
    ocAssert(sizeY > 0);

    // The main framebuffer must have been created first because the input attachments of the final composition framebuffer are the
    // outputs of the main framebuffer.
    ocAssert(pRT->colorImageView != 0);
    ocAssert(pRT->dsImageView != 0);

    if ((outputImageCount == 0 || outputImageCount > 3) || pOutputImages == NULL) return OC_INVALID_ARGS;
    memcpy(pRT->outputImages, pOutputImages, sizeof(*pOutputImages) * outputImageCount);
    pRT->outputImageCount = 0;

    // Output image views.
    for (uint32_t iOutputImage = 0; iOutputImage < outputImageCount; ++iOutputImage) {
        VkResult vkresult = ocvkCreateSimpleImageView(pWorld->pGraphics->device, pOutputImages[iOutputImage], VK_IMAGE_VIEW_TYPE_2D, outputImageFormat, ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1), NULL, &pRT->outputImageViews[iOutputImage]);
        if (vkresult != VK_SUCCESS) {
            goto on_error;
        }

        VkImageView attachments[2];
        attachments[0] = pRT->outputImageViews[iOutputImage];
        attachments[1] = pRT->colorImageView;

        VkFramebufferCreateInfo fbInfo;
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.pNext = NULL;
        fbInfo.flags = 0;
        fbInfo.renderPass = pWorld->pGraphics->renderPass_FinalComposite_Image;     // <-- renderPass_FinalComposite_Image and renderPass_FinalComposite_Window should be compatible.
        fbInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
        fbInfo.pAttachments = attachments;
        fbInfo.width = sizeX;
        fbInfo.height = sizeY;
        fbInfo.layers = 1;
        vkresult = vkCreateFramebuffer(pWorld->pGraphics->device, &fbInfo, NULL, &pRT->outputFBs[iOutputImage]);
        if (vkresult != VK_SUCCESS) {
            goto on_error;
        }


        // Command buffers for initial transition before being drawn to.
        vkresult = ocvkAllocateCommandBuffers(pWorld->pGraphics->device, pWorld->pGraphics->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &pRT->cbPreTransition_Outputs[iOutputImage]);
        if (vkresult != VK_SUCCESS) {
            goto on_error;
        }

        ocvkBeginCommandBuffer(pRT->cbPreTransition_Outputs[iOutputImage], 0, NULL);
        {
            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = NULL;
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            if (pRT->pSwapchain != NULL) {
                barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;   
            } else {
                barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = pWorld->pGraphics->queueFamilyIndex;
            barrier.dstQueueFamilyIndex = pWorld->pGraphics->queueFamilyIndex;
            barrier.image = pOutputImages[iOutputImage];
            barrier.subresourceRange = ocvkImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
            vkCmdPipelineBarrier(pRT->cbPreTransition_Outputs[iOutputImage], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        }
        vkEndCommandBuffer(pRT->cbPreTransition_Outputs[iOutputImage]);


        pRT->outputImageCount += 1;
    }

    return OC_SUCCESS;

on_error:
    ocGraphicsWorldUninitRT_OutputFramebuffers(pWorld, pRT);
    return OC_ERROR;
}

OC_PRIVATE ocResult ocGraphicsWorldAllocAndInitRT(ocGraphicsWorld* pWorld, ocGraphicsRT** ppRT, ocGraphicsSwapchain* pSwapchain, ocGraphicsImage* pImage, uint32_t sizeX, uint32_t sizeY, uint32_t outputImageCount, VkImage* pOutputImages, VkFormat outputImageFormat)
{
    if (ppRT == NULL) return OC_INVALID_ARGS;
    *ppRT = NULL;   // Safety.

    if (pWorld == NULL) return OC_INVALID_ARGS;
    if (pSwapchain != NULL && pImage != NULL) return OC_INVALID_ARGS;   // It's not valid for a swapchain _and_ an image to be the output of an RT.

    ocGraphicsRT* pRT = (ocGraphicsRT*)ocCalloc(1, sizeof(*pRT));
    if (pRT == NULL) return OC_OUT_OF_MEMORY;

    ocResult result = ocGraphicsRTInitBase(pWorld, pRT);
    if (result != OC_SUCCESS) {
        ocFree(pRT);
        return result;
    }

    pRT->pSwapchain = pSwapchain;
    pRT->pImage = pImage;
    pRT->sizeX = sizeX;
    pRT->sizeY = sizeY;
    pRT->projection = glm::mat4()/* * ocMakeMat4_VulkanClipCorrection()*/;
    pRT->projection = glm::mat4()/* * ocMakeMat4_VulkanClipCorrection()*/;
    pRT->view = glm::mat4();

    // Main framebuffer.
    result = ocGraphicsWorldAllocAndInitRT_MainFramebuffer(pWorld, pRT, sizeX, sizeY);
    if (result != OC_SUCCESS) {
        ocFree(pRT);
        return result;
    }

    // Output framebuffers.
    result = ocGraphicsWorldAllocAndInitRT_OutputFramebuffers(pWorld, pRT, sizeX, sizeY, outputImageCount, pOutputImages, outputImageFormat);
    if (result != OC_SUCCESS) {
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        ocFree(pRT);
        return result;
    }


    // Uniform buffer.
    VkBufferCreateInfo uboInfo;
    uboInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uboInfo.pNext = NULL;
    uboInfo.flags = 0;
    uboInfo.size = sizeof(glm::mat4)*2;
    uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uboInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uboInfo.queueFamilyIndexCount = 0;
    uboInfo.pQueueFamilyIndices = NULL;
    VkResult vkresult = vkCreateBuffer(pWorld->pGraphics->device, &uboInfo, NULL, &pRT->uniformBuffer);
    if (vkresult != VK_SUCCESS) {
        ocGraphicsWorldUninitRT_OutputFramebuffers(pWorld, pRT);
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        ocFree(pRT);
        return result;
    }

    vkresult = ocvkAllocateAndBindBufferMemory(pWorld->pGraphics->physicalDevice, pWorld->pGraphics->device, pRT->uniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, NULL, &pRT->uniformBufferMemory);
    if (vkresult != VK_SUCCESS) {
        vkDestroyBuffer(pWorld->pGraphics->device, pRT->uniformBuffer, NULL);
        ocGraphicsWorldUninitRT_OutputFramebuffers(pWorld, pRT);
        ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);
        ocFree(pRT);
        return result;
    }

    // Permanently map the uniform buffer data.
    vkMapMemory(pWorld->pGraphics->device, pRT->uniformBufferMemory, 0, sizeof(glm::mat4)*2, 0, &pRT->pUniformBufferData);
    memcpy(ocOffsetPtr(pRT->pUniformBufferData, 0),                 &pRT->projection, sizeof(pRT->projection));
    memcpy(ocOffsetPtr(pRT->pUniformBufferData, sizeof(glm::mat4)), &pRT->view,       sizeof(pRT->view));


    pRT->uniformBufferDescriptor.buffer = pRT->uniformBuffer;
    pRT->uniformBufferDescriptor.offset = 0;
    pRT->uniformBufferDescriptor.range  = sizeof(glm::mat4)*2;

    *ppRT = pRT;
    return OC_SUCCESS;
}

ocResult ocGraphicsWorldCreateRTFromSwapchain(ocGraphicsWorld* pWorld, ocGraphicsSwapchain* pSwapchain, ocGraphicsRT** ppRT)
{
    uint32_t sizeX;
    uint32_t sizeY;
    ocGraphicsGetSwapchainSize(pSwapchain->pGraphics, pSwapchain, &sizeX, &sizeY);

    ocResult result = ocGraphicsWorldAllocAndInitRT(pWorld, ppRT, pSwapchain, NULL, (uint32_t)sizeX, (uint32_t)sizeY, pSwapchain->vkSwapchainImageCount, pSwapchain->vkSwapchainImages, VK_FORMAT_R8G8B8A8_UNORM);
    if (result != OC_SUCCESS) {
        return result;
    }

    ocGraphicsRT* pRT = *ppRT;
    pRT->pSwapchain = pSwapchain;
    pRT->pImage = NULL;

    result = ocGraphicsWorldAddRT(pWorld, pRT);
    if (result != OC_SUCCESS) {
        ocGraphicsRTUninitBase(pRT);
        return result;
    }

    return OC_SUCCESS;
}

ocResult ocGraphicsWorldCreateRTFromImage(ocGraphicsWorld* pWorld, ocGraphicsImage* pImage, ocGraphicsRT** ppRT)
{
    ocResult result = ocGraphicsWorldAllocAndInitRT(pWorld, ppRT, NULL, pImage, pImage->sizeX, pImage->sizeY, 1, &pImage->imageVK, VK_FORMAT_R8G8B8A8_UNORM);     // <-- Change this to the format of the image for robustness?
    if (result != OC_SUCCESS) {
        return result;
    }

    ocGraphicsRT* pRT = *ppRT;
    pRT->pSwapchain = NULL;
    pRT->pImage = pImage;

    result = ocGraphicsWorldAddRT(pWorld, pRT);
    if (result != OC_SUCCESS) {
        ocGraphicsRTUninitBase(pRT);
        return result;
    }

    return OC_SUCCESS;
}

void ocGraphicsWorldDeleteRT(ocGraphicsWorld* pWorld, ocGraphicsRT* pRT)
{
    if (pWorld == NULL || pRT == NULL) return;

    // TODO: Destroy the uniform buffer.
    vkUnmapMemory(pWorld->pGraphics->device, pRT->uniformBufferMemory);

    ocGraphicsWorldUninitRT_OutputFramebuffers(pWorld, pRT);
    ocGraphicsWorldUninitRT_MainFramebuffer(pWorld, pRT);

    ocGraphicsWorldRemoveRT(pWorld, pRT);
    ocGraphicsRTUninitBase(pRT);
}


OC_PRIVATE ocResult ocGraphicsObjectInit(ocGraphicsObject* pObject, ocGraphicsWorld* pWorld, ocGraphicsObjectType type)
{
    ocResult result = ocGraphicsObjectBaseInit(pWorld, type, pObject);
    if (result != OC_SUCCESS) {
        ocFree(pObject);
        return result;
    }

    pObject->_position  = glm::vec4(0, 0, 0, 0);
    pObject->_rotation  = glm::quat(1, 0, 0, 0);
    pObject->_scale     = glm::vec4(1, 1, 1, 1);
    pObject->_transform = glm::mat4();

    // Uniform buffer.
    VkBufferCreateInfo uboInfo; // TODO: ocvkBufferCreateInfo() / ocvkDefaultBufferCreateInfo() / ocvkSimpleBufferCreateInfo()
    uboInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uboInfo.pNext = NULL;
    uboInfo.flags = 0;
    uboInfo.size = sizeof(glm::mat4)*2;
    uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uboInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uboInfo.queueFamilyIndexCount = 0;
    uboInfo.pQueueFamilyIndices = NULL;
    VkResult vkresult = vkCreateBuffer(pWorld->pGraphics->device, &uboInfo, NULL, &pObject->uniformBuffer);
    if (vkresult != VK_SUCCESS) {
        return result;
    }

    vkresult = ocvkAllocateAndBindBufferMemory(pWorld->pGraphics->physicalDevice, pWorld->pGraphics->device, pObject->uniformBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, NULL, &pObject->uniformBufferMemory);
    if (vkresult != VK_SUCCESS) {
        vkDestroyBuffer(pWorld->pGraphics->device, pObject->uniformBuffer, NULL);
        return result;
    }

    // Permanently map the uniform buffer data.
    vkMapMemory(pWorld->pGraphics->device, pObject->uniformBufferMemory, 0, sizeof(glm::mat4)*2, 0, &pObject->_pUniformBufferData);
    memcpy(ocOffsetPtr(pObject->_pUniformBufferData, 0), glm::value_ptr(pObject->_transform), sizeof(glm::mat4));


    pObject->uniformBufferDescriptor.buffer = pObject->uniformBuffer;
    pObject->uniformBufferDescriptor.offset = 0;
    pObject->uniformBufferDescriptor.range  = sizeof(glm::mat4);

    return OC_SUCCESS;
}

ocResult ocGraphicsWorldCreateMeshObject(ocGraphicsWorld* pWorld, ocGraphicsMesh* pMesh, ocGraphicsObject** ppObjectOut)
{
    if (pWorld == NULL || pMesh == NULL || ppObjectOut == NULL) return OC_INVALID_ARGS;

    ocGraphicsObject* pObject = ocMallocObject(ocGraphicsObject);
    if (pObject == NULL) {
        return OC_OUT_OF_MEMORY;
    }

    ocResult result = ocGraphicsObjectInit(pObject, pWorld, ocGraphicsObjectType_Mesh);
    if (result != OC_SUCCESS) {
        ocFree(pObject);
        return result;
    }

    pObject->data.mesh.pResource = pMesh;

    // Add the object to the world. Should this be done explicitly at a higher level for consistency with ocWorld?
    pWorld->pObjects->push_back(pObject);

    *ppObjectOut = pObject;
    return OC_SUCCESS;
}

void ocGraphicsWorldDeleteObject(ocGraphicsWorld* pWorld, ocGraphicsObject* pObject)
{
    if (pWorld == NULL || pObject == NULL) return;

    pWorld->pObjects->erase(std::remove(pWorld->pObjects->begin(), pWorld->pObjects->end(), pObject), pWorld->pObjects->end());

    // TODO: Destroy the uniform buffer.
    vkUnmapMemory(pWorld->pGraphics->device, pObject->uniformBufferMemory);
    ocFree(pObject);
}


void ocGraphicsWorldSetObjectPosition(ocGraphicsWorld* pWorld, ocGraphicsObject* pObject, const glm::vec3 &position)
{
    ocGraphicsWorldSetObjectTransform(pWorld, pObject, position, pObject->_rotation, glm::vec3(pObject->_scale));
}

void ocGraphicsWorldSetObjectRotation(ocGraphicsWorld* pWorld, ocGraphicsObject* pObject, const glm::quat &rotation)
{
    ocGraphicsWorldSetObjectTransform(pWorld, pObject, glm::vec3(pObject->_position), rotation, glm::vec3(pObject->_scale));
}

void ocGraphicsWorldSetObjectScale(ocGraphicsWorld* pWorld, ocGraphicsObject* pObject, const glm::vec3 &scale)
{
    ocGraphicsWorldSetObjectTransform(pWorld, pObject, glm::vec3(pObject->_position), pObject->_rotation, scale);
}

void ocGraphicsWorldSetObjectTransform(ocGraphicsWorld* pWorld, ocGraphicsObject* pObject, const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &scale)
{
    if (pWorld == NULL || pObject == NULL) return;
    pObject->_position  = glm::vec4(position, 0);
    pObject->_rotation  = rotation;
    pObject->_scale     = glm::vec4(scale, 1);
    pObject->_transform = ocMakeMat4(pObject->_position, pObject->_rotation, pObject->_scale);

    // Update uniform buffer.
    memcpy(ocOffsetPtr(pObject->_pUniformBufferData, 0), glm::value_ptr(pObject->_transform), sizeof(glm::mat4));
}

