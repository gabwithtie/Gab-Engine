//PUT ODR DEFINES HERE AS THIS FILE IS THE FIRST FILE TO BE COMPILED IN THE RENDER PIPELINE
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "RenderPipeline.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "Editor/gbe_editor.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = false;
#endif

using namespace gbe;

gbe::RenderPipeline* gbe::RenderPipeline::Instance;

gbe::RenderPipeline* gbe::RenderPipeline::Get_Instance() {
	return gbe::RenderPipeline::Instance;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation ERROR: " << pCallbackData->pMessage << std::endl;
		throw std::runtime_error("Vulkan validation layer error encountered: " + std::string(pCallbackData->pMessage));
    }
    else {
        std::cerr << "validation log: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

gbe::RenderPipeline::RenderPipeline(gbe::Window* window, Vector2Int dimensions)
{
    if(this->Instance != nullptr)
		throw std::runtime_error("RenderPipeline instance already exists!");

    this->Instance = this;

	this->window = window;
	this->resolution = dimensions;

#pragma region SDL x VULKAN init
    auto implemented_window = static_cast<SDL_Window*>(window->Get_implemented_window());
    
    //EXTENSIONS
    uint32_t SDLextensionCount;
    const char** SDLextensionNames = 0;
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, nullptr);
    SDLextensionNames = new const char* [SDLextensionCount];
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, SDLextensionNames);
    std::vector<const char*> allextensions(SDLextensionNames, SDLextensionNames + SDLextensionCount);
    if (enableValidationLayers) {
        allextensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //APP INFO
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    bool validationlayerssupported = true;

    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            validationlayerssupported = false;
        }
    }

    if (enableValidationLayers && !validationlayerssupported) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    
	//INSTANCE INFO
    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.pNext = nullptr;
    instInfo.flags = 0;
    instInfo.ppEnabledLayerNames = nullptr;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(allextensions.size());;
    instInfo.ppEnabledExtensionNames = allextensions.data();

    //DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
    if (enableValidationLayers) {
        instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instInfo.ppEnabledLayerNames = validationLayers.data();

        debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_create_info.pfnUserCallback = debugCallback;
        debug_messenger_create_info.pUserData = nullptr; // Optional

        instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;
    }
    else {
        instInfo.enabledLayerCount = 0;

		instInfo.pNext = nullptr;
    }
    
    //INSTANCE
    auto instresult = vkCreateInstance(&instInfo, nullptr, &vkInst);
    if (instresult != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    //SDL SURFACE
    SDL_Vulkan_CreateSurface(implemented_window, vkInst, &vksurface);

    //DEBUG
    if (enableValidationLayers) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInst, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(vkInst, &debug_messenger_create_info, nullptr, &this->debugMessenger);
        }
        else {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    vkInst.Init();

	//Asset Loaders
    this->shaderloader.AssignSelfAsLoader();
    this->shaderloader.PassDependencies(&this->vkdevice, &this->swapchainExtent, &this->renderPass);
	
    this->meshloader.AssignSelfAsLoader();
	this->meshloader.PassDependencies(&this->vkdevice, &this->vkphysicalDevice);
	
    this->materialloader.AssignSelfAsLoader();
    this->materialloader.PassDependencies(&this->shaderloader);

    this->textureloader.AssignSelfAsLoader();
    this->textureloader.PassDependencies(&this->vkdevice, &this->vkphysicalDevice);

    //Assigning pipeline specific variables
    this->PipelineVariables.insert_or_assign("VkInstance", &vkInst);
    this->PipelineVariables.insert_or_assign("VkSurfaceKHR", &vksurface);
    this->PipelineVariables.insert_or_assign("VkDevice", &vkdevice);
    this->PipelineVariables.insert_or_assign("VkPhysicalDevice", &vkphysicalDevice);
    this->PipelineVariables.insert_or_assign("VkRenderPass", &renderPass);
    this->PipelineVariables.insert_or_assign("VkQueue_graphics", &graphicsQueue);
    this->PipelineVariables.insert_or_assign("VkQueue_present", &presentQueue);
}

void gbe::RenderPipeline::AssignEditor(Editor* _editor)
{
    this->editor = _editor;
}

void RenderPipeline::SetCameraShader(asset::Shader* camshader) {
	
}

void gbe::RenderPipeline::SetResolution(Vector2Int newresolution) {
	this->resolution = newresolution;
}


//========== RUNTIME THINGS ==========//
bool RenderPipeline::TryPushLight(gfx::Light* data, bool priority) {

    if (this->lights_this_frame.size() == this->maxlights)
        return false;

    if (priority) {
        this->lights_this_frame.push_front(data);
        return true;
    }

    this->lights_this_frame.push_back(data);
    return true;
}

void gbe::RenderPipeline::RenderFrame(Matrix4 viewmat, Matrix4 projmat, float& nearclip, float& farclip)
{
	//Syncronization
    vkWaitForFences(this->vkdevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult aquireResult = vkAcquireNextImageKHR(this->vkdevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (aquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        this->RefreshPipelineObjects();
        return;
    }
    else if (aquireResult != VK_SUCCESS && aquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(this->vkdevice, 1, &inFlightFences[currentFrame]);

	auto currentCommandBuffer = commandBuffers[currentFrame];
    vkResetCommandBuffer(currentCommandBuffer, 0);

#pragma region command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    //Start render pass
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];

    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = this->swapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    float clear_brightness = 0.3f;
    clearValues[0].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(currentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    //Render an object
    projmat[1][1] = -projmat[1][1]; //Flip Y axis for Vulkan

    for (const auto &pair : this->drawcalls)
    {
		const auto& draworder = pair.first;
		const auto& drawcallbatch = pair.second;

        for (const auto& drawcall : drawcallbatch)
        {
            //SYNC FIRST
            drawcall->SyncMaterialData(currentFrame);

            //USE SHADER
            auto shaderasset = drawcall->get_material()->getShader();
            const auto& currentshaderdata = shaderloader.GetAssetData(shaderasset);
            vkCmdBindPipeline(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipeline);

            VkViewport viewport{};
            viewport.width = static_cast<float>(this->swapchainExtent.width);
            viewport.height = static_cast<float>(this->swapchainExtent.height);
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = this->swapchainExtent;
            vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

            //RENDER MESH
            const auto& curmesh = this->meshloader.GetAssetData(drawcall->get_mesh());
			
            //UPDATE GLOBAL UBO
            for (int dc_i = 0; dc_i < drawcall->get_call_count(); dc_i++) {
                auto& callinstance = drawcall->get_call_instance(dc_i);

                drawcall->ApplyOverride<Matrix4>(callinstance.model, "model", currentFrame, callinstance);
                drawcall->ApplyOverride<Matrix4>(projmat, "proj", currentFrame, callinstance);
                drawcall->ApplyOverride<Matrix4>(viewmat, "view", currentFrame, callinstance);

                VkBuffer vertexBuffers[] = { curmesh.vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(currentCommandBuffer, 0, 1, vertexBuffers, offsets);


                std::vector<VkDescriptorSet> bindingsets;
                for (auto& set : callinstance.allocdescriptorSets)
                {
                    bindingsets.push_back(set[currentFrame]);
                }

                vkCmdBindDescriptorSets(currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipelineLayout, 0, bindingsets.size(), bindingsets.data(), 0, nullptr);

                vkCmdBindIndexBuffer(currentCommandBuffer, curmesh.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(currentCommandBuffer, static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
            }
        }
    }

    //RECORDING
    if (recording) {
        const auto frame_data = this->ScreenShot();

        // Decode the image from the buffer into a cv::Mat
        cv::Mat image = cv::imdecode(frame_data, cv::IMREAD_COLOR);

        // Check if the image was successfully decoded
        if (image.empty()) {
            std::runtime_error("Failed to decode image!");
        }

        video_frames.push_back(image);
    }

    //EDITOR/GUI PASS
    this->editor->RenderPass(currentCommandBuffer);

    vkCmdEndRenderPass(currentCommandBuffer);

    if (vkEndCommandBuffer(currentCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
#pragma endregion

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        this->RefreshPipelineObjects();
    }
    else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    this->currentFrame = (this->currentFrame + 1) % this->MAX_FRAMES_IN_FLIGHT;
}

std::vector<unsigned char> gbe::RenderPipeline::ScreenShot(bool write_file) {
    // Source for the copy is the last rendered swapchain image
    VkImage srcImage = swapChainImages[currentFrame];

    // Create the linear tiled destination image to copy to and to read the memory from
    VkImageCreateInfo imageCreateCI{};
	imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
    // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
    imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateCI.extent.width = this->resolution.x;
    imageCreateCI.extent.height = this->resolution.y;
    imageCreateCI.extent.depth = 1;
    imageCreateCI.arrayLayers = 1;
    imageCreateCI.mipLevels = 1;
    imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Create the image
    VkImage dstImage;
    if (vkCreateImage(this->vkdevice, &imageCreateCI, nullptr, &dstImage) != VK_SUCCESS){
		throw new std::runtime_error("failed to create image for screenshot!");
    }
    // Create memory to back up the image
    VkMemoryRequirements memRequirements{};
    VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkDeviceMemory dstImageMemory;
    vkGetImageMemoryRequirements(this->vkdevice, dstImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    // Memory must be host visible to copy from
    memAllocInfo.memoryTypeIndex = this->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vkAllocateMemory(this->vkdevice, &memAllocInfo, nullptr, &dstImageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate memory for screenshot image!");
	}
    if (vkBindImageMemory(this->vkdevice, dstImage, dstImageMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image memory for screenshot image!");
    }

    // Do the actual blit from the swapchain image to our host visible destination image
    VkCommandBuffer copyCmd;
	beginSingleTimeCommands(copyCmd);

    // Transition destination image to transfer destination layout
    this->insertImageMemoryBarrier(
        copyCmd,
        dstImage,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // Transition swapchain image from present to transfer source layout
    this->insertImageMemoryBarrier(
        copyCmd,
        srcImage,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    
    // Otherwise use image copy (requires us to manually flip components)
    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = this->resolution.x;
    imageCopyRegion.extent.height = this->resolution.y;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(
        copyCmd,
        srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion);

    // Transition destination image to general layout, which is the required layout for mapping the image memory later on
    this->insertImageMemoryBarrier(
        copyCmd,
        dstImage,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // Transition back the swap chain image after the blit is done
    this->insertImageMemoryBarrier(
        copyCmd,
        srcImage,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    endSingleTimeCommands(copyCmd);

    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(this->vkdevice, dstImage, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    const char* data;
    vkMapMemory(this->vkdevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
    data += subResourceLayout.offset;

    std::stringstream s_out = std::stringstream();

    // ppm header
    s_out << "P6\n" << this->resolution.x << "\n" << this->resolution.y << "\n" << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
    
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), chosenSurfaceFormat.format) != formatsBGR.end());
    

        // ppm binary pixel data
    for (uint32_t y = 0; y < this->resolution.y; y++)
    {
        unsigned int* row = (unsigned int*)data;
        for (uint32_t x = 0; x < this->resolution.x; x++)
        {
            if (colorSwizzle)
            {
                s_out.write((char*)row + 2, 1);
                s_out.write((char*)row + 1, 1);
                s_out.write((char*)row, 1);
            }
            else
            {
                s_out.write((char*)row, 3);
            }
            row++;
        }
        data += subResourceLayout.rowPitch;
    }

    auto out_string = s_out.str();
    auto out_data = std::vector<unsigned char>(out_string.begin(), out_string.end());

    // Clean up resources
    vkUnmapMemory(this->vkdevice, dstImageMemory);
    vkFreeMemory(this->vkdevice, dstImageMemory, nullptr);
    vkDestroyImage(this->vkdevice, dstImage, nullptr);

    if (write_file) {
        auto file = std::ofstream("out/ss.ppm", std::ios::out | std::ios::binary);
        file << out_string;
        file.close();
    }

    return out_data;
}

void gbe::RenderPipeline::StartRecording()
{
    this->recording = true;
}

void gbe::RenderPipeline::StopRecording()
{
    //Export File
    if (this->recording) {
        std::string codecstr = "avc1";
        const unsigned int codec = (((((codecstr[3] << 8) | codecstr[2]) << 8) | codecstr[1]) << 8) | codecstr[0];
        std::string outputFileName = "latest_recording.mp4";
        double fps = 24.0;
        cv::Size frameSize(this->resolution.x, this->resolution.y);

        // Initialize VideoWriter
        cv::VideoWriter outputVideo;
        outputVideo.open(outputFileName, codec, fps, frameSize, true);

        // Write frames to video
        for (const auto& frame : this->video_frames) {
            // Create a destination Mat for the flipped image
            cv::Mat mirrored_image;

            cv::flip(frame, mirrored_image, 0);
            outputVideo.write(mirrored_image);
        }

        // Release the VideoWriter
        outputVideo.release();

        std::cout << "Video saved successfully to " << outputFileName << std::endl;
    }

    this->recording = false;
}

void gbe::RenderPipeline::ToggleRecording()
{
    if (this->recording)
        this->StopRecording();
    else
        this->StartRecording();
}

gbe::gfx::DrawCall* gbe::RenderPipeline::RegisterDrawCall(asset::Mesh* mesh, asset::Material* material)
{
	auto newdrawcall = new DrawCall(mesh, material, &shaderloader.GetAssetData(material->getShader()), this->MAX_FRAMES_IN_FLIGHT, &this->vkdevice, 0);

	if (this->drawcalls.find(newdrawcall->get_order()) == this->drawcalls.end()) {
		this->drawcalls.insert_or_assign(newdrawcall->get_order(), std::vector<DrawCall*>{ newdrawcall });
	}
	else {
		this->drawcalls[newdrawcall->get_order()].push_back(newdrawcall);
	}

    return newdrawcall;
}

DrawCall* gbe::RenderPipeline::RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material)
{
    this->default_drawcall = RegisterDrawCall(mesh, material);

    return this->default_drawcall;
}

DrawCall* gbe::RenderPipeline::GetDefaultDrawCall()
{
    return this->default_drawcall;
}

void* gbe::RenderPipeline::GetPipelineVariable(std::string id)
{
    if (this->PipelineVariables.find(id) == this->PipelineVariables.end()) {
        throw std::runtime_error("Variable does not exist.");
    }

    return this->PipelineVariables.at(id);
}
