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

	//1: INSTANCE INFO
    this->vulkanInstance = new vulkan::Instance(resolution.x, resolution.y, allextensions, enableValidationLayers);
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
    renderer->StartShadowPass();

    auto frustrum_corners = Matrix4::get_frustrum_corners(frameinfo.projmat, frameinfo.viewmat);
    auto frustrum_center = Matrix4::get_frustrum_center(frustrum_corners);

    // Loop through each light that casts a shadow.
    for (size_t lightIndex = 0; lightIndex < frameinfo.lightdatas.size(); lightIndex++)
    {
        const auto& light = frameinfo.lightdatas[lightIndex];

        // Update the light's view and projection matrices.
        Matrix4 lightViewMat = light->GetViewMatrix();
        Matrix4 lightProjMat = light->GetProjectionMatrix();

        if (light->type == Light::DIRECTIONAL) {
            auto backtrack_dist = 20.0f;
            auto overshoot_dist = 100.0f;

            const auto lightView = glm::lookAt(
                frustrum_center - (light->direction * backtrack_dist),
                frustrum_center,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float maxZ = std::numeric_limits<float>::lowest();
            for (const auto& v : frustrum_corners)
            {
                const auto trf = lightView * v;
                minX = std::min(minX, trf.x);
                maxX = std::max(maxX, trf.x);
                minY = std::min(minY, trf.y);
                maxY = std::max(maxY, trf.y);
                maxZ = std::max(maxZ, trf.z);
            }

            lightViewMat = lightView;
            lightProjMat = glm::ortho(minX, maxX, minY, maxY, 0.0f, maxZ + overshoot_dist);
        }

        for (const auto& call_ptr : this->sortedcalls[-1]) {
            const auto& callinstance = this->calls[call_ptr][-1];

            //USE SHADER
            auto shaderasset = callinstance.drawcall->get_material()->Get_load_data().shader;
            const auto& currentshaderdata = shaderloader.GetAssetData(shaderasset);
            vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipeline);

            callinstance.drawcall->SyncMaterialData(vulkanInstance->GetCurrentFrameIndex(), callinstance);

            //RENDER MESH
            const auto& curmesh = this->meshloader.GetAssetData(callinstance.drawcall->get_mesh());

            //UPDATE OVERRIDES (light)
            callinstance.ApplyOverride<Matrix4>(callinstance.model, "model", vulkanInstance->GetCurrentFrameIndex());
            callinstance.ApplyOverride<Matrix4>(lightViewMat, "view", vulkanInstance->GetCurrentFrameIndex());
            callinstance.ApplyOverride<Matrix4>(lightProjMat, "proj", vulkanInstance->GetCurrentFrameIndex());

            // Use push constants to specify the array layer to write to.
            uint32_t pushConstantData = lightIndex;
            vkCmdPushConstants(vulkanInstance->GetCurrentCommandBuffer()->GetData(), currentshaderdata.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &pushConstantData);

            // Bind vertex/index buffers and draw.
            VkBuffer vertexBuffers[] = { curmesh.vertexBuffer->GetData() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);
            
            std::vector<VkDescriptorSet> bindingsets;
            for (auto& set : callinstance.allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
            {
                bindingsets.push_back(set.second);
            }

            vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipelineLayout, 0, bindingsets.size(), bindingsets.data(), 0, nullptr);
            vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), curmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
        }
    }

    //==================SECOND PASS [1]========================//
    renderer->TransitionToMainPass();

    for (const auto& call_ptr : sortedcalls[0])
    {
        const auto& callinstance = this->calls[call_ptr][0];

        //USE SHADER
        auto shaderasset = callinstance.drawcall->get_material()->Get_load_data().shader;
        const auto& currentshaderdata = shaderloader.GetAssetData(shaderasset);
        vkCmdBindPipeline(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipeline);

        callinstance.drawcall->SyncMaterialData(vulkanInstance->GetCurrentFrameIndex(), callinstance);

        // Use push constants to specify the array layer to write to.
        uint32_t pushConstantData = 0;
        vkCmdPushConstants(vulkanInstance->GetCurrentCommandBuffer()->GetData(), currentshaderdata.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &pushConstantData);

        //RENDER MESH
        const auto& curmesh = this->meshloader.GetAssetData(callinstance.drawcall->get_mesh());

        //UPDATE OVERRIDES
        callinstance.ApplyOverride<Matrix4>(callinstance.model, "model", vulkanInstance->GetCurrentFrameIndex());
        callinstance.ApplyOverride<Matrix4>(projmat, "proj", vulkanInstance->GetCurrentFrameIndex());
        callinstance.ApplyOverride<Matrix4>(frameinfo.viewmat, "view", vulkanInstance->GetCurrentFrameIndex());

        callinstance.ApplyOverride<Vector3>(frameinfo.camera_pos, "camera_pos", vulkanInstance->GetCurrentFrameIndex());

        TextureData shadowmaptex = {};
        renderer->Get_image_data(shadowmaptex.textureImage, shadowmaptex.textureImageView, shadowmaptex.textureSampler);
        callinstance.ApplyOverride<TextureData>(shadowmaptex, "shadow_tex", vulkanInstance->GetCurrentFrameIndex());

        size_t light_index = 0;
        for (const auto& light : frameinfo.lightdatas)
        {
            if (light_index == renderer->Get_max_lights())
                break;

            callinstance.ApplyOverride<Matrix4>(light->GetViewMatrix(), "light_view", vulkanInstance->GetCurrentFrameIndex(), light_index);
            callinstance.ApplyOverride<Matrix4>(light->GetProjectionMatrix(), "light_proj", vulkanInstance->GetCurrentFrameIndex(), light_index);
            callinstance.ApplyOverride<Vector3>(light->color, "light_color", vulkanInstance->GetCurrentFrameIndex(), light_index);
            
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
            callinstance.ApplyOverride<Vector3>(Vector3(0), "light_color", vulkanInstance->GetCurrentFrameIndex(), light_index);
        }

        VkBuffer vertexBuffers[] = { curmesh.vertexBuffer->GetData() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vulkanInstance->GetCurrentCommandBuffer()->GetData(), 0, 1, vertexBuffers, offsets);

        std::vector<VkDescriptorSet> bindingsets;
        for (auto& set : callinstance.allocdescriptorSets_perframe[vulkanInstance->GetCurrentFrameIndex()])
        {
            bindingsets.push_back(set.second);
        }

        vkCmdBindDescriptorSets(vulkanInstance->GetCurrentCommandBuffer()->GetData(), VK_PIPELINE_BIND_POINT_GRAPHICS, currentshaderdata.pipelineLayout, 0, bindingsets.size(), bindingsets.data(), 0, nullptr);
        vkCmdBindIndexBuffer(vulkanInstance->GetCurrentCommandBuffer()->GetData(), curmesh.indexBuffer->GetData(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(vulkanInstance->GetCurrentCommandBuffer()->GetData(), static_cast<uint32_t>(curmesh.loaddata->indices.size()), 1, 0, 0, 0);
    }

    //EDITOR/GUI PASS
    if (editor != nullptr) {
        this->editor->RenderPass(vulkanInstance->GetCurrentCommandBuffer());
    }

    renderer->EndMainPass();
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
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), vulkan::PhysicalDevice::GetActive()->Get_swapchainFormat().format) != formatsBGR.end());
    
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
    auto newdrawcall = new DrawCall(mesh, material, &Instance->shaderloader.GetAssetData(material->Get_load_data().shader), Instance->vulkanInstance->Get_maxFrames());

    Instance->drawcalls.push_back(newdrawcall);

    return newdrawcall;
}

DrawCall* gbe::RenderPipeline::RegisterDefaultDrawCall(asset::Mesh* mesh, asset::Material* material)
{
    Instance->default_drawcall = RegisterDrawCall(mesh, material);

    return Instance->default_drawcall;
}

DrawCall* gbe::RenderPipeline::GetDefaultDrawCall()
{
    return Instance->default_drawcall;
}

gbe::Matrix4* gbe::RenderPipeline::RegisterCall(void* instance_id, DrawCall* drawcall, Matrix4 matrix, int order)
{
    bool exists = this->calls.find(instance_id) != this->calls.end();
    bool exists_index = false;
    
    if (exists)
        exists_index = this->calls[instance_id].find(order) != this->calls[instance_id].end();

    if (exists_index)
		throw new std::runtime_error("Instance already registered!");

    //FORCE UPDATE OVERRIDES
    for (size_t m_i = 0; m_i < drawcall->get_material()->getOverrideCount(); m_i++)
    {
        std::string id;
        auto& overridedata = drawcall->get_material()->getOverride(m_i, id);
        overridedata.registered_change = false; // Reset handled change for new call
    }

    CallInstance newinst{};
    newinst.drawcall = drawcall;
    newinst.order = order;
    newinst.shaderdata = drawcall->get_shaderdata();

    //BUFFERS
    for (const auto& block : drawcall->get_shaderdata()->uniformblocks)
    {
        auto newblockbuffer = CallInstance::UniformBlockBuffer{};
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

        newinst.uniformBuffers.push_back(newblockbuffer);
    }

    //Textures
    for (const auto& field : drawcall->get_shaderdata()->uniformfields)
    {
        if (field.type == asset::Shader::UniformFieldType::TEXTURE)
        {
            std::vector<CallInstance::UniformTexture> texture_arr = {};

            for (size_t i = 0; i < field.array_size; i++)
            {
                CallInstance::UniformTexture newtexture{};
                newtexture.array_index = i;
                newtexture.texture_name = field.name;
                // Leave image view and sampler for the texture default
                auto defaultImage = TextureLoader::GetDefaultImage();
                newtexture.imageView = defaultImage.textureImageView;
                newtexture.sampler = defaultImage.textureSampler;

                texture_arr.push_back(newtexture);
            }

            newinst.uniformTextures.insert_or_assign(field.name, texture_arr);
        }
    }

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

    newinst.descriptorPool = new vulkan::DescriptorPool(poolSizes, vulkanInstance->Get_maxFrames() * drawcall->get_shaderdata()->descriptorSetLayouts.size());

    //DESCRIPTOR SETS
    auto setcount = drawcall->get_shaderdata()->descriptorSetLayouts.size();
    newinst.allocdescriptorSets_perframe.resize(vulkanInstance->Get_maxFrames());

    for (const auto& pair : drawcall->get_shaderdata()->descriptorSetLayouts) {
        auto& descriptorSetLayout = drawcall->get_shaderdata()->descriptorSetLayouts[pair.first];

        std::vector<VkDescriptorSetLayout> layoutsperframe(vulkanInstance->Get_maxFrames(), descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = newinst.descriptorPool->GetData();
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
            newinst.allocdescriptorSets_perframe[f_i][pair.first] = sets[f_i];
        }
    }

    for (size_t f_i = 0; f_i < vulkanInstance->Get_maxFrames(); f_i++) {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};

        std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos(newinst.uniformBuffers.size());
        for (size_t i_i = 0; i_i < newinst.uniformBuffers.size(); i_i++)
        {
            const auto& uniformblock = newinst.uniformBuffers[i_i];

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
            descriptorWrite.dstSet = newinst.allocdescriptorSets_perframe[f_i][blockinfo.set];
            descriptorWrite.dstBinding = blockinfo.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = blockinfo.array_size;
            descriptorWrite.pBufferInfo = bufferInfos[i_i].data();

            descriptorWrites.push_back(descriptorWrite);
        }

        std::vector< std::vector<VkDescriptorImageInfo>> imageInfos(newinst.uniformTextures.size());
        unsigned int info_index = 0;
        for (const auto& pair: newinst.uniformTextures)
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
            descriptorWrite.dstSet = newinst.allocdescriptorSets_perframe[f_i][fieldinfo.set];
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

    //COMMITTING
    if(!exists)
        this->calls[instance_id] = {};

    this->calls[instance_id][order] = newinst;
    if (sortedcalls.find(order) == sortedcalls.end())
        sortedcalls[order] = std::vector<void*>();
    sortedcalls.find(order)->second.push_back(instance_id);

    return &this->calls[instance_id][order].model;
}

void gbe::RenderPipeline::UnRegisterCall(void* instance_id, int order)
{
    auto iter = Instance->calls.find(instance_id);
    bool exists = iter != Instance->calls.end();
    
    auto callinst = iter->second[order];

    if (!exists)
		throw new std::runtime_error("CallInstance does not exist!");

    for (size_t i = 0; i < callinst.uniformBuffers.size(); i++)
    {
        for (size_t j = 0; j < callinst.uniformBuffers[i].uboPerFrame.size(); j++)
        {
            vulkan::VirtualDevice::GetActive()->UnMapMemory(callinst.uniformBuffers[i].uboPerFrame[j]->GetMemory());
            delete callinst.uniformBuffers[i].uboPerFrame[j];
        }
    }

    auto& sortvec = Instance->sortedcalls.find(callinst.order)->second;
    for (size_t i = 0; i < sortvec.size(); i++)
    {
        if (sortvec[i] == instance_id) {
            sortvec.erase(sortvec.begin() + i);
            break;
        }
    }

    Instance->calls.erase(instance_id);
}
