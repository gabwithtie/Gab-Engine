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

gbe::RenderPipeline::RenderPipeline(gbe::Window& window, Vector2Int dimensions):
    window(window)
{
    if(this->Instance != nullptr)
		throw std::runtime_error("RenderPipeline instance already exists!");

    this->Instance = this;

    //===================WINDOW INIT==================
	this->window = window;
	this->screen_resolution = dimensions;

    auto implemented_window = static_cast<SDL_Window*>(window.Get_implemented_window());
    
    //===================SDL INIT==================
    uint32_t SDLextensionCount;
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, nullptr);
    std::vector<const char*> allextensions(SDLextensionCount);
    SDL_Vulkan_GetInstanceExtensions(implemented_window, &SDLextensionCount, allextensions.data());
    
    if (enableValidationLayers)
        vulkan::ValidationLayers::Check(allextensions);

	//1: INSTANCE INFO
    this->vulkanInstance = new vulkan::Instance(screen_resolution.x, screen_resolution.y, allextensions, enableValidationLayers);
    vulkan::Instance::SetActive(this->vulkanInstance);
	
    //2: SDL SURFACE
    VkSurfaceKHR sdl_surface;
    SDL_Vulkan_CreateSurface(implemented_window, vulkanInstance->GetData(), &sdl_surface);
    vulkan::Surface* surfaceobject = new vulkan::Surface(sdl_surface, vulkanInstance->GetData());
    //This new() call will be managed by the vulkan instance

    //3: Vulkan Initialization
    vulkanInstance->Init_Surface(surfaceobject);
    this->renderer = new vulkan::ForwardRenderer(); //freed by vulkan instance
    vulkanInstance->Init_Renderer(this->renderer);

	//Asset Loaders
    this->shaderloader.AssignSelfAsLoader();
    this->meshloader.AssignSelfAsLoader();
    this->materialloader.AssignSelfAsLoader();
    this->textureloader.AssignSelfAsLoader();

    UpdateReferences();

    VkDeviceSize vbufferSize = sizeof(asset::data::Vertex) * maxlines;
    this->line_vertexBuffer = new vulkan::Buffer(vbufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void gbe::RenderPipeline::UpdateReferences()
{
    TextureData mainpass_tex = {
    .textureImageView = this->renderer->Get_mainpass()->Get_color()->GetView(),
    .textureSampler = new vulkan::Sampler()
    };
    textureloader.RegisterExternal("mainpass", mainpass_tex);

    TextureData shadowpass_tex = {
    .textureImageView = this->renderer->Get_shadowpass()->Get_color()->GetView(),
    .textureSampler = new vulkan::Sampler()
    };
    textureloader.RegisterExternal("shadowpass", shadowpass_tex);

    for (size_t i = 0; i < renderer->Get_max_lights(); i++)
    {
        TextureData shadowmap_tex = {
        .textureImageView = this->renderer->Get_shadowmap_layer(i),
        .textureSampler = new vulkan::Sampler()
        };
        textureloader.RegisterExternal("shadowmap_" + std::to_string(i), shadowmap_tex);
    }
}

void gbe::RenderPipeline::DrawLine(Vector3 a, Vector3 b)
{
    Instance->lines_this_frame.push_back({
            .pos = a
        });
    Instance->lines_this_frame.push_back({
            .pos = b
        });
}

void gbe::RenderPipeline::RenderFrame(const FrameRenderInfo& frameinfo)
{
    if (window.isMinimized())
        return;

    //=============VULKAN PREP============//
    vulkanInstance->PrepareFrame();

    //Render DrawCalls -> Engine-specific code
    auto projmat = frameinfo.projmat;
    projmat[1][1] = -projmat[1][1]; //Flip Y axis for Vulkan

    //==================FIRST PASS [0]========================//
    // Loop through each light that casts a shadow.
    for (size_t lightIndex = 0; lightIndex < renderer->Get_max_lights(); lightIndex++)
    {
        if (lightIndex == frameinfo.lightdatas.size())
            break;

        renderer->StartShadowPass(lightIndex);

        const auto& light = frameinfo.lightdatas[lightIndex];

        // Update the light's view and projection matrices.
        light->UpdateContext(frameinfo.viewmat, frameinfo.projmat_lightusage);
        Matrix4 lightViewMat = light->GetViewMatrix();
        Matrix4 lightProjMat = light->GetProjectionMatrix();

        for (const auto& shaderset : sortedcalls[-1])
        {
            //USE SHADER
            const auto& drawcall = shaderset.first;
            const auto& currentshaderdata = drawcall->get_shaderdata();
            vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata->pipeline);

            drawcall->SyncMaterialData(vulkanInstance->GetCurrentFrameIndex());

            //RENDER MESH
            const auto& curmesh = this->meshloader.GetAssetData(drawcall->get_mesh());

            VkBuffer vertexBuffers[] = { curmesh.vertexBuffer->GetData() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), curmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);

            for (auto& set : drawcall->allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
            {
                vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata->pipelineLayout, set.first, 1, &set.second, 0, nullptr);
            }

            for (const auto& call_ptr : shaderset.second)
            {
                struct shadow_pushconstants {
                    Matrix4 projview;
                    Matrix4 model;
                };

                shadow_pushconstants pushconstants = {
                    .projview = lightProjMat * lightViewMat,
                    .model = this->matrix_map[call_ptr]
                };

                vkCmdPushConstants(vulkanInstance->GetCurrentCommandBuffer()->GetData(), currentshaderdata->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadow_pushconstants), &pushconstants);
                vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
            }
        }

        renderer->EndShadowPass();
    }

    //==================SECOND PASS [1]========================//
    renderer->StartMainPass();

    //===============LINE PASS
    if (lines_this_frame.size() > 0) {
        VkDeviceSize vbufferSize = sizeof(asset::data::Vertex) * lines_this_frame.size();
        vulkan::Buffer stagingBuffer(vbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        void* vdata;
        vulkan::VirtualDevice::GetActive()->MapMemory(stagingBuffer.GetMemory(), 0, vbufferSize, 0, &vdata);
        memcpy(vdata, this->lines_this_frame.data(), (size_t)vbufferSize);
        vulkan::VirtualDevice::GetActive()->UnMapMemory(stagingBuffer.GetMemory());
        vulkan::Buffer::CopyBuffer(&stagingBuffer, line_vertexBuffer, vbufferSize);

        auto lineshaderasset = this->line_call->get_material()->Get_load_data().shader;
        const auto& lineshader = shaderloader.GetAssetData(lineshaderasset);
        vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, lineshader.pipeline);

        this->line_call->ApplyOverride<Matrix4>(Matrix4(1), "model", vulkanInstance->GetCurrentFrameIndex());
        this->line_call->ApplyOverride<Matrix4>(projmat, "proj", vulkanInstance->GetCurrentFrameIndex());
        this->line_call->ApplyOverride<Matrix4>(frameinfo.viewmat, "view", vulkanInstance->GetCurrentFrameIndex());
        this->line_call->ApplyOverride<Vector3>(frameinfo.camera_pos, "camera_pos", vulkanInstance->GetCurrentFrameIndex());

        for (auto& set : this->line_call->allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
        {
            vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, lineshader.pipelineLayout, set.first, 1, &set.second, 0, nullptr);
        }

        VkBuffer vertexBuffers[] = { line_vertexBuffer->GetData() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);
        vkCmdDraw(vulkanInstance->GetCurrentCommandBuffer()->GetData(), lines_this_frame.size(), 1, 0, 0);
    }
    //=================END OF LINE PASS

    //=================SKYBOX PASS
    {
        const auto& skyboxmesh = this->meshloader.GetAssetData(this->skybox_call->get_mesh());
        auto skyboxshaderasset = this->skybox_call->get_material()->Get_load_data().shader;
        const auto& skyboxshader = shaderloader.GetAssetData(skyboxshaderasset);
        vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxshader.pipeline);

        this->skybox_call->ApplyOverride<Matrix4>(Matrix4(1), "model", vulkanInstance->GetCurrentFrameIndex());
        this->skybox_call->ApplyOverride<Matrix4>(projmat, "proj", vulkanInstance->GetCurrentFrameIndex());
        this->skybox_call->ApplyOverride<Matrix4>(frameinfo.viewmat, "view", vulkanInstance->GetCurrentFrameIndex());
        this->skybox_call->ApplyOverride<Vector3>(frameinfo.camera_pos, "camera_pos", vulkanInstance->GetCurrentFrameIndex());
        for (auto& set : this->skybox_call->allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
        {
            vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxshader.pipelineLayout, set.first, 1, &set.second, 0, nullptr);
        }
        VkBuffer vertexBuffers[] = { skyboxmesh.vertexBuffer->GetData() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), skyboxmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(skyboxmesh.loaddata->indices.size()), 1, 0, 0, 0);
    }
    //=================END OF SKYBOX PASS

    for (const auto& shaderset : sortedcalls[0])
    {
        //USE SHADER
        const auto& drawcall = shaderset.first;
        const auto& currentshaderdata = drawcall->get_shaderdata();
        vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata->pipeline);

        drawcall->SyncMaterialData(vulkanInstance->GetCurrentFrameIndex());

        //RENDER MESH
        const auto& curmesh = this->meshloader.GetAssetData(drawcall->get_mesh());

        drawcall->ApplyOverride<Matrix4>(projmat, "proj", vulkanInstance->GetCurrentFrameIndex());
        drawcall->ApplyOverride<Matrix4>(frameinfo.viewmat, "view", vulkanInstance->GetCurrentFrameIndex());

        drawcall->ApplyOverride<Vector3>(frameinfo.camera_pos, "camera_pos", vulkanInstance->GetCurrentFrameIndex());

        TextureData shadowmaptex = {
        .textureImageView = renderer->Get_shadowpass()->Get_depth()->GetView(),
        .textureSampler = renderer->Get_sampler()
        };
        drawcall->ApplyOverride<TextureData>(shadowmaptex, "shadow_tex", vulkanInstance->GetCurrentFrameIndex());

        size_t light_index = 0;
        for (size_t lightIndex = 0; lightIndex < renderer->Get_max_lights(); lightIndex++)
        {
            if (lightIndex >= frameinfo.lightdatas.size())
            {
                drawcall->ApplyOverride<Vector3>(Vector3(0), "light_color", vulkanInstance->GetCurrentFrameIndex(), light_index);
                continue;
            }

            const auto& light = frameinfo.lightdatas[lightIndex];
            auto lightProjMat = light->cache_projmat;

            drawcall->ApplyOverride<Matrix4>(light->cache_viewmat, "light_view", vulkanInstance->GetCurrentFrameIndex(), light_index);
            drawcall->ApplyOverride<Matrix4>(lightProjMat, "light_proj", vulkanInstance->GetCurrentFrameIndex(), light_index);
            drawcall->ApplyOverride<Vector3>(light->color, "light_color", vulkanInstance->GetCurrentFrameIndex(), light_index);
            drawcall->ApplyOverride<float>(light->bias_min, "bias_min", vulkanInstance->GetCurrentFrameIndex(), light_index);
            drawcall->ApplyOverride<float>(light->bias_mult, "bias_mult", vulkanInstance->GetCurrentFrameIndex(), light_index);
            drawcall->ApplyOverride<float>(1, "shadow_strength", vulkanInstance->GetCurrentFrameIndex(), light_index);

            switch (light->type)
            {
            case Light::DIRECTIONAL:
                break;
            }

            light_index++;
        }

        //RESET DATA OF THE REST OF THE LIGHT SETS
        for (size_t i = light_index; i < renderer->Get_max_lights(); i++)
        {
            drawcall->ApplyOverride<Vector3>(Vector3(0), "light_color", vulkanInstance->GetCurrentFrameIndex(), light_index);
        }

        VkBuffer vertexBuffers[] = { curmesh.vertexBuffer->GetData() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), curmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);

        std::vector<VkDescriptorSet> bindingsets;
        for (auto& set : drawcall->allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
        {
            bindingsets.push_back(set.second);
        }
        vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata->pipelineLayout, 0, bindingsets.size(), bindingsets.data(), 0, nullptr);

        for (const auto& call_ptr : shaderset.second)
        {
            vkCmdPushConstants(vulkanInstance->GetCurrentCommandBuffer()->GetData(), currentshaderdata->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4), &this->matrix_map[call_ptr]);
            vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
        }
    }

    renderer->TransitionToScreenPass();

    //EDITOR/GUI PASS
    if (editor != nullptr) {
        this->editor->RenderPass(vulkanInstance->GetCurrentCommandBuffer());
    }

    renderer->EndScreenPass();

    vulkanInstance->PushFrame();

    if (!handled_resolution_change) {
        vulkanInstance->RefreshPipelineObjects(this->viewport_resolution.x, this->viewport_resolution.y);
        UpdateReferences();
        handled_resolution_change = true;
    }

    lines_this_frame.clear();

}

std::vector<unsigned char> gbe::RenderPipeline::ScreenShot(bool write_file) {
    // Source for the copy is the last rendered swapchain image
    vulkan::Image* srcImage = vulkan::SwapChain::GetActive()->GetImage(vulkanInstance->GetCurrentSwapchainImage());
    vulkan::Image dstImage(this->screen_resolution.x, this->screen_resolution.y, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
   
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
    imageCopyRegion.extent.width = this->screen_resolution.x;
    imageCopyRegion.extent.height = this->screen_resolution.y;
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
    s_out << "P6\n" << this->screen_resolution.x << "\n" << this->screen_resolution.y << "\n" << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), vulkan::PhysicalDevice::GetActive()->Get_swapchainFormat().format) != formatsBGR.end());
    
    // ppm binary pixel data
    for (uint32_t y = 0; y < this->screen_resolution.y; y++)
    {
        unsigned int* row = (unsigned int*)data;
        for (uint32_t x = 0; x < this->screen_resolution.x; x++)
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

gbe::gfx::DrawCall* gbe::RenderPipeline::RegisterDrawCall(asset::Mesh* mesh, asset::Material* material, int order)
{
    for (const auto& pair : Instance->sortedcalls[order])
    {
        const auto& drawcall = pair.first;

        if (drawcall->get_mesh() == mesh && drawcall->get_material() == material) {
            return drawcall;
        }
    }

    auto newdrawcall = new DrawCall(mesh, material, &Instance->shaderloader.GetAssetData(material->Get_load_data().shader), order);

    return newdrawcall;
}

DrawCall* gbe::RenderPipeline::RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material)
{
    Instance->default_drawcall = RegisterDrawCall(mesh, material, 0);

    return Instance->default_drawcall;
}

DrawCall* gbe::RenderPipeline::GetDefaultDrawCall()
{
    return Instance->default_drawcall;
}

gbe::Matrix4* gbe::RenderPipeline::RegisterInstance(void* instance_id, DrawCall* drawcall, Matrix4 matrix, int order)
{
    bool exists_matrix = this->matrix_map.find(instance_id) != this->matrix_map.end();
    bool exists_instance = false;
    
    bool ordermap_exists = this->sortedcalls.find(order) != this->sortedcalls.end();

    if (ordermap_exists) {
        bool drawcall_exists = this->sortedcalls[order].find(drawcall) != this->sortedcalls[order].end();

        if (drawcall_exists) {
            for (const auto& instance_ptr : this->sortedcalls[order][drawcall])
            {
                if (instance_ptr == instance_id) {
                    exists_instance = true;
                    break;
                }
            }

            if (!exists_instance) {
                this->sortedcalls[order][drawcall].push_back(instance_id);
            }

            return &this->matrix_map[instance_id];
        }
    }

    //FORCE UPDATE OVERRIDES
    for (size_t m_i = 0; m_i < drawcall->get_material()->getOverrideCount(); m_i++)
    {
        std::string id;
        auto& overridedata = drawcall->get_material()->getOverride(m_i, id);
        overridedata.registered_change = false; // Reset handled change for new call
    }

    this->PrepareCall(drawcall, order);

    //COMMITTING
    if(!exists_matrix)
        this->matrix_map[instance_id] = Matrix4();

    if (sortedcalls.find(order) == sortedcalls.end())
        sortedcalls[order] = std::unordered_map<DrawCall*, std::vector<void*>>();
    if (sortedcalls[order].find(drawcall) == sortedcalls[order].end())
        sortedcalls[order][drawcall] = std::vector<void*>();
    
    sortedcalls[order][drawcall].push_back(instance_id);

    return &this->matrix_map[instance_id];
}

void gbe::RenderPipeline::PrepareCall(DrawCall* drawcall, int order)
{
    //BUFFERS
    for (const auto& block : drawcall->get_shaderdata()->uniformblocks)
    {
        auto newblockbuffer = DrawCall::UniformBlockBuffer{};
        newblockbuffer.block_name = block.name;

        // Create uniform buffer for each block
        VkDeviceSize bufferSize = block.block_size_aligned * block.array_size;

        newblockbuffer.uboPerFrame.resize(vulkanInstance->Get_maxFrames());
        newblockbuffer.uboMappedPerFrame.resize(vulkanInstance->Get_maxFrames());

        for (size_t i = 0; i < vulkanInstance->Get_maxFrames(); i++) {
            //MM_note: will be freed by unregister call instance.
            newblockbuffer.uboPerFrame[i] = new vulkan::Buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, i);

            vulkan::VirtualDevice::GetActive()->MapMemory(newblockbuffer.uboPerFrame[i]->GetMemory(), 0, bufferSize, 0, &newblockbuffer.uboMappedPerFrame[i]);
        }

        drawcall->uniformBuffers.push_back(newblockbuffer);
    }

    //Textures
    for (const auto& field : drawcall->get_shaderdata()->uniformfields)
    {
        if (field.type == asset::Shader::UniformFieldType::TEXTURE)
        {
            std::vector<DrawCall::UniformTexture> texture_arr = {};

            for (size_t i = 0; i < field.array_size; i++)
            {
                DrawCall::UniformTexture newtexture{};
                newtexture.array_index = i;
                newtexture.texture_name = field.name;
                // Leave image view and sampler for the texture default
                auto defaultImage = TextureLoader::GetDefaultImage();
                newtexture.imageView = defaultImage.textureImageView;
                newtexture.sampler = defaultImage.textureSampler;

                texture_arr.push_back(newtexture);
            }

            drawcall->uniformTextures.insert_or_assign(field.name, texture_arr);
        }
    }

    auto setcount = drawcall->get_shaderdata()->descriptorSetLayouts.size();
    drawcall->allocdescriptorSets_perframe.resize(vulkanInstance->Get_maxFrames());

    if (setcount > 0)
    {
        //Descriptor Pool
        std::map<VkDescriptorType, uint32_t> descriptorTypeCounts;

        for (const auto& set : drawcall->get_shaderdata()->binding_sets) {
            for (const auto& binding : set.second) {
                descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
            }
        }

        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto& pair : descriptorTypeCounts) {
            poolSizes.push_back({
                pair.first,
                pair.second * vulkanInstance->Get_maxFrames() // Multiply by frames in flight
                });
        }

        drawcall->descriptorPool = new vulkan::DescriptorPool(poolSizes, vulkanInstance->Get_maxFrames() * drawcall->get_shaderdata()->descriptorSetLayouts.size());

        //DESCRIPTOR SETS
        for (const auto& pair : drawcall->get_shaderdata()->descriptorSetLayouts) {
            auto& descriptorSetLayout = drawcall->get_shaderdata()->descriptorSetLayouts[pair.first];

            std::vector<VkDescriptorSetLayout> layoutsperframe(vulkanInstance->Get_maxFrames(), descriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = drawcall->descriptorPool->GetData();
            allocInfo.descriptorSetCount = static_cast<uint32_t>(vulkanInstance->Get_maxFrames());
            allocInfo.pSetLayouts = layoutsperframe.data();

            std::vector<VkDescriptorSet> sets(vulkanInstance->Get_maxFrames());
            auto alloc_result = vkAllocateDescriptorSets(vulkan::VirtualDevice::GetActive()->GetData(), &allocInfo, sets.data());
            if (alloc_result != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            // Store each set in the correct frame/set slot
            for (size_t f_i = 0; f_i < vulkanInstance->Get_maxFrames(); f_i++) {
                // The set index (set_i) should correspond to the Vulkan set number
                vulkan::DebugObjectName::NameVkObject(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)sets[f_i], "DS_" + std::to_string(f_i));
                drawcall->allocdescriptorSets_perframe[f_i][pair.first] = sets[f_i];
            }
        }

        for (size_t f_i = 0; f_i < vulkanInstance->Get_maxFrames(); f_i++) {
            std::vector<VkWriteDescriptorSet> descriptorWrites{};

            std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(drawcall->uniformBuffers.size());
            for (size_t i_i = 0; i_i < drawcall->uniformBuffers.size(); i_i++)
            {
                const auto& uniformblock = drawcall->uniformBuffers[i_i];

                ShaderData::ShaderBlock blockinfo{};

                bool found_block = drawcall->get_shaderdata()->FindUniformBlock(uniformblock.block_name, blockinfo);
                if (!found_block)
                    throw std::runtime_error("Failed to find uniform block: " + uniformblock.block_name);

                bufferInfos[i_i].resize(blockinfo.array_size);
                for (uint32_t j = 0; j < blockinfo.array_size; ++j) {
                    bufferInfos[i_i][j].buffer = uniformblock.uboPerFrame[f_i]->GetData();
                    bufferInfos[i_i][j].offset = j * blockinfo.block_size_aligned;
                    bufferInfos[i_i][j].range = blockinfo.block_size;
                }

                //CREATE THE WRITE DATA
                VkWriteDescriptorSet descriptorWrite{};

                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = drawcall->allocdescriptorSets_perframe[f_i][blockinfo.set];
                descriptorWrite.dstBinding = blockinfo.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = blockinfo.array_size;
                descriptorWrite.pBufferInfo = bufferInfos[i_i].data();

                descriptorWrites.push_back(descriptorWrite);
            }

            std::vector< std::vector<VkDescriptorImageInfo>> imageInfos(drawcall->uniformTextures.size());
            unsigned int info_index = 0;
            for (const auto& pair : drawcall->uniformTextures)
            {
                const auto& arr_size = pair.second.size();
                imageInfos[info_index].resize(arr_size);

                for (size_t texindex = 0; texindex < arr_size; texindex++) {
                    const auto& uniformtex = pair.second[texindex];
                    imageInfos[info_index][texindex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[info_index][texindex].imageView = uniformtex.imageView->GetData();
                    imageInfos[info_index][texindex].sampler = uniformtex.sampler->GetData();
                }
                //FIND THE BINDING INDEX
                ShaderData::ShaderField fieldinfo;
                ShaderData::ShaderBlock blockinfo;
                drawcall->get_shaderdata()->FindUniformField(pair.first, fieldinfo, blockinfo);
                //CREATE THE WRITE DATA
                VkWriteDescriptorSet descriptorWrite{};

                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = drawcall->allocdescriptorSets_perframe[f_i][fieldinfo.set];
                descriptorWrite.dstBinding = fieldinfo.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = arr_size;
                descriptorWrite.pImageInfo = imageInfos[info_index].data();

                descriptorWrites.push_back(descriptorWrite);

                info_index++;
            }

            vkUpdateDescriptorSets(vulkan::VirtualDevice::GetActive()->GetData(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
    else
    {
        drawcall->descriptorPool = nullptr;
    }
}

void gbe::RenderPipeline::UnRegisterCall(void* instance_id)
{
    auto iter = Instance->matrix_map.find(instance_id);
    bool exists = iter != Instance->matrix_map.end();
    
    if (!exists)
		throw new std::runtime_error("CallInstance does not exist!");

    for (const auto& drawcallmap : Instance->sortedcalls)
    for (const auto& pair : drawcallmap.second) {
        auto drawcall = pair.first;
        auto instance_list = pair.second;

        for (size_t i = 0; i < instance_list.size(); i++)
        {
            if (instance_list[i] == instance_id) {
                instance_list.erase(instance_list.begin() + i);
                break;
            }
        }

        if (instance_list.size() == 0) {
            delete drawcall;
        }
    }

    Instance->matrix_map.erase(instance_id);
}
