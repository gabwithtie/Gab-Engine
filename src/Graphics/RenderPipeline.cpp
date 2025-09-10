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
const bool enableValidationLayers = true;
#endif

using namespace gbe;

gbe::RenderPipeline* gbe::RenderPipeline::Instance;

gbe::RenderPipeline* gbe::RenderPipeline::Get_Instance() {
	return gbe::RenderPipeline::Instance;
}

gbe::RenderPipeline::RenderPipeline(gbe::Window& window, Vector2Int dimensions):
    window(window)
{
    if(this->Instance != nullptr)
		throw std::runtime_error("RenderPipeline instance already exists!");

    this->Instance = this;

    //===================WINDOW INIT==================
	this->window = window;
	this->resolution = dimensions;

    auto implemented_window = static_cast<SDL_Window*>(window.Get_implemented_window());
    
    //===================SDL INIT==================
    uint32_t SDLextensionCount;
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, nullptr);
    std::vector<const char*> allextensions(SDLextensionCount);
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, allextensions.data());
    
    if (enableValidationLayers)
        vulkan::ValidationLayers::Check(allextensions);

	//INSTANCE INFO
    this->vulkanInstance = new vulkan::Instance(resolution.x, resolution.y, allextensions, enableValidationLayers);
    vulkan::Instance::SetActive(this->vulkanInstance);

    //SDL SURFACE
    VkSurfaceKHR sdl_surface;
    SDL_Vulkan_CreateSurface(implemented_window, vulkanInstance->GetData(), &sdl_surface);
    vulkan::Surface* surfaceobject = new vulkan::Surface(sdl_surface, vulkanInstance->GetData());
    //This new() call will be managed by the vulkan instance

    //Vulkan Initialization
    vulkanInstance->Init(surfaceobject);

	//Asset Loaders
    this->shaderloader.AssignSelfAsLoader();
    this->meshloader.AssignSelfAsLoader();
    this->materialloader.AssignSelfAsLoader();
    this->textureloader.AssignSelfAsLoader();
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
    if (window.isMinimized())
        return;

    //Vulkan preparation
    vulkanInstance->PrepareFrame();
    
    //Render DrawCalls -> Engine-specific code
    projmat[1][1] = -projmat[1][1]; //Flip Y axis for Vulkan

    for (const auto &pair : this->drawcalls)
    {
		const auto& draworder = pair.first;
		const auto& drawcallbatch = pair.second;

        for (const auto& drawcall : drawcallbatch)
        {
            //SYNC FIRST
            drawcall->SyncMaterialData(vulkanInstance->GetCurrentFrameIndex());

            //USE SHADER
            auto shaderasset = drawcall->get_material()->getShader();
            const auto& currentshaderdata = shaderloader.GetAssetData(shaderasset);
            vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipeline);

            VkViewport viewport{};
            viewport.width = static_cast<float>(vulkan::SwapChain::GetActive()->GetExtent().width);
            viewport.height = static_cast<float>(vulkan::SwapChain::GetActive()->GetExtent().height);
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = vulkan::SwapChain::GetActive()->GetExtent();
            vkCmdSetScissor(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, &scissor);

            //RENDER MESH
            const auto& curmesh = this->meshloader.GetAssetData(drawcall->get_mesh());
			
            //UPDATE GLOBAL UBO
            for (int dc_i = 0; dc_i < drawcall->get_call_count(); dc_i++) {
                auto& callinstance = drawcall->get_call_instance(dc_i);

                drawcall->ApplyOverride<Matrix4>(callinstance.model, "model", vulkanInstance->GetCurrentFrameIndex(), callinstance);
                drawcall->ApplyOverride<Matrix4>(projmat, "proj", vulkanInstance->GetCurrentFrameIndex(), callinstance);
                drawcall->ApplyOverride<Matrix4>(viewmat, "view", vulkanInstance->GetCurrentFrameIndex(), callinstance);

                VkBuffer vertexBuffers[] = { curmesh.vertexBuffer->GetData()};
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);

                std::vector<VkDescriptorSet> bindingsets;
                for (auto& set : callinstance.allocdescriptorSets)
                {
                    bindingsets.push_back(set[vulkanInstance->GetCurrentFrameIndex()]);
                }

                vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipelineLayout, 0, bindingsets.size(), bindingsets.data(), 0, nullptr);

                vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), curmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
            }
        }
    }

    //EDITOR/GUI PASS
    if (editor != nullptr) {
        this->editor->RenderPass(vulkanInstance->GetCurrentCommandBuffer());
    }

    vulkanInstance->PushFrame();
}

std::vector<unsigned char> gbe::RenderPipeline::ScreenShot(bool write_file) {
    // Source for the copy is the last rendered swapchain image
    vulkan::Image* srcImage = vulkan::SwapChain::GetActive()->GetImage(vulkanInstance->GetCurrentSwapchainImage());
    vulkan::Image dstImage(this->resolution.x, this->resolution.y, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
   
    // Do the actual blit from the swapchain image to our host visible destination image
    vulkan::CommandBufferSingle copyCmd;
    copyCmd.Begin();

    // Transition destination image to transfer destination layout
    vulkan::MemoryBarrier::Insert(
        copyCmd.GetData(),
        &dstImage,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // Transition swapchain image from present to transfer source layout
    vulkan::MemoryBarrier::Insert(
        copyCmd.GetData(),
        srcImage,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    //Use image copy (requires us to manually flip components)
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
        copyCmd.GetData(),
        srcImage->GetData(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage.GetData(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion);

    // Transition destination image to general layout, which is the required layout for mapping the image memory later on
    vulkan::MemoryBarrier::Insert(
        copyCmd.GetData(),
        &dstImage,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // Transition back the swap chain image after the blit is done
    vulkan::MemoryBarrier::Insert(
        copyCmd.GetData(),
        srcImage,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    copyCmd.End();

    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(vulkan::VirtualDevice::GetActive()->GetData(), dstImage.GetData(), &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    const char* data;
    vulkan::VirtualDevice::GetActive()->MapMemory(dstImage.Get_imageMemory(), 0, VK_WHOLE_SIZE, 0, (void**)&data);
    data += subResourceLayout.offset;

    std::stringstream s_out = std::stringstream();

    // ppm header
    s_out << "P6\n" << this->resolution.x << "\n" << this->resolution.y << "\n" << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), vulkan::SwapChain::GetActive()->GetFormat().format) != formatsBGR.end());
    
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

    if (write_file) {
        auto file = std::ofstream("out/ss.ppm", std::ios::out | std::ios::binary);
        file << out_string;
        file.close();
    }

    return out_data;
}

gbe::gfx::DrawCall* gbe::RenderPipeline::RegisterDrawCall(asset::Mesh* mesh, asset::Material* material)
{
	auto newdrawcall = new DrawCall(mesh, material, &shaderloader.GetAssetData(material->getShader()), vulkanInstance->Get_maxFrames(), 0);

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
