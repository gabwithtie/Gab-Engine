#pragma once

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"
#include "../../Objects/SwapChain.h"
#include "../../Objects/Instance.h"
#include "../../Objects/Sampler.h"

#include "../../Utility/MemoryBarrier.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        uint32_t shadow_map_resolution = 512;
        uint32_t max_lights = 10;

        Image* shadow_image_array = nullptr;
        ImageView* shadow_imageview = nullptr;
        Sampler* shadow_sampler = nullptr;
        Framebuffer* shadow_buffer = nullptr;

        std::vector<ImageView*> shadow_imageview_layers;
        std::vector<Sampler*> shadow_sampler_layers;

        AttachmentDictionary attachments_shadow;

        RenderPass* shadow_pass = nullptr;

    public:
        inline void Get_shadowmap_image_data(
            Image*& shadow_image_array_ptr,
            ImageView*& shadow_imageview_ptr,
            Sampler*& shadow_sampler_ptr
        ) {
            shadow_image_array_ptr = shadow_image_array;
            shadow_imageview_ptr = shadow_imageview;
            shadow_sampler_ptr = shadow_sampler;
        }

        inline void Get_shadowmap_layer_data(
            ImageView*& shadow_imageview_ptr,
            Sampler*& shadow_sampler_ptr,
            uint32_t index
        ) {
            shadow_imageview_ptr = shadow_imageview_layers[index];
            shadow_sampler_ptr = shadow_sampler_layers[index];
        }

        inline uint32_t Get_max_lights()
        {
            return max_lights;
        }


        inline ForwardRenderer() {
            shadow_image_array = new Image(
                shadow_map_resolution, shadow_map_resolution,
                PhysicalDevice::GetActive()->GetDepthFormat(),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                max_lights
            );
            shadow_imageview = new ImageView(shadow_image_array, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
            shadow_sampler = new Sampler();
            
            for (size_t i = 0; i < max_lights; i++)
            {
                shadow_imageview_layers.push_back(new ImageView(shadow_image_array, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, i));
                shadow_sampler_layers.push_back(new Sampler());
            }


            VkAttachmentDescription shadowmap_attachment = {};
            shadowmap_attachment.format = PhysicalDevice::GetActive()->GetDepthFormat(); // Use a suitable depth format
            shadowmap_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            shadowmap_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            shadowmap_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            shadowmap_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            shadowmap_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            shadowmap_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            shadowmap_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachments_shadow.AddAttachment("shadowmap", shadowmap_attachment);
            
            VkAttachmentDescription depth_attachment = {};
            depth_attachment.format = PhysicalDevice::GetActive()->GetDepthFormat();
            depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments_main.AddAttachment("depth", depth_attachment);

            VkAttachmentDescription color_attachment = {};
            color_attachment.format = PhysicalDevice::GetActive()->Get_swapchainFormat().format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            attachments_main.AddAttachment("color", color_attachment);
            attachments_shadow.AddAttachment("color", color_attachment);
        }

        inline ~ForwardRenderer() {

        }

        inline void SetBounds(uint32_t x, uint32_t y) {
            //BOUNDS
            VkViewport viewport{};
            viewport.width = x;
            viewport.height = y;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = {x, y};
            vkCmdSetScissor(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), 0, 1, &scissor);

        }

        inline void StartShadowPass() {
            VkRenderPassBeginInfo shadowPassBeginInfo{};
            
            SetBounds(shadow_map_resolution, shadow_map_resolution);

            shadowPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            shadowPassBeginInfo.renderPass = shadow_pass->GetData();
            shadowPassBeginInfo.framebuffer = shadow_buffer->GetData();

            shadowPassBeginInfo.renderArea.offset = { 0, 0 };
            shadowPassBeginInfo.renderArea.extent = {shadow_map_resolution, shadow_map_resolution};

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].depthStencil = { 1.0f, 0 };
            clearValues[1].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };

            shadowPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            shadowPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), &shadowPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        inline void TransitionToMainPass() {
            vkCmdEndRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData());

            SetBounds(
                vulkan::SwapChain::GetActive()->GetExtent().width,
                vulkan::SwapChain::GetActive()->GetExtent().height
            );
            
            MemoryBarrier::Insert(
                Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
                shadow_image_array,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = max_lights
                }
            );

            //RENDER PASS START
            VkRenderPassBeginInfo mainPassBeginInfo{};
            mainPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            mainPassBeginInfo.renderPass = main_pass->GetData();
            mainPassBeginInfo.framebuffer = Instance::GetActive()->GetCurrentSwapchainBuffer()->GetData();

            mainPassBeginInfo.renderArea.offset = { 0, 0 };
            mainPassBeginInfo.renderArea.extent = SwapChain::GetActive()->GetExtent();

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].depthStencil = { 1.0f, 0 };
            clearValues[1].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };

            mainPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            mainPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), &mainPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        inline void EndMainPass() {
            vkCmdEndRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData());
        }

        inline void Refresh() override {
            if (shadow_buffer != nullptr)
                delete shadow_buffer;
            if (shadow_pass != nullptr)
                delete shadow_pass;
            if (main_pass != nullptr)
                delete main_pass;

            //References
            VkAttachmentReference shadowMapDepthRef = attachments_shadow.GetRef("shadowmap", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            VkAttachmentReference colorRef = attachments_main.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkAttachmentReference finalDepthRef = attachments_main.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            //=======================SHADOW PASS========================//
            VkSubpassDescription shadowSubpass = {};
            shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            shadowSubpass.colorAttachmentCount = 0;
            shadowSubpass.pDepthStencilAttachment = &shadowMapDepthRef;

            // Render Pass Info
            VkRenderPassCreateInfo passinfo_shadow = {};
            passinfo_shadow.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            passinfo_shadow.attachmentCount = attachments_shadow.GetSize();
            passinfo_shadow.pAttachments = attachments_shadow.GetArrayPtr();
            passinfo_shadow.subpassCount = 1;
            passinfo_shadow.pSubpasses = &shadowSubpass;
            passinfo_shadow.dependencyCount = 0;

            VkRenderPass pass_shadow;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo_shadow, nullptr, &pass_shadow) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
            shadow_pass = new RenderPass(pass_shadow);
            RenderPass::SetActive("shadow", shadow_pass);

            //====================MAIN PASS====================//
            VkSubpassDescription subpass_main{};
            subpass_main.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_main.colorAttachmentCount = 1;
            subpass_main.pColorAttachments = &colorRef;
            subpass_main.pDepthStencilAttachment = &finalDepthRef;

            std::vector<VkSubpassDependency> dependencies_main = {};
            dependencies_main.resize(1);
            //Depth test dependency
            dependencies_main[0] = {};
            dependencies_main[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies_main[0].dstSubpass = 0;
            dependencies_main[0].srcAccessMask = 0;
            dependencies_main[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dependencies_main[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies_main[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            // Now, create the render pass.
            VkRenderPassCreateInfo passinfo_main = {};
            passinfo_main.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            passinfo_main.attachmentCount = attachments_main.GetSize();
            passinfo_main.pAttachments = attachments_main.GetArrayPtr();
            passinfo_main.subpassCount = 1;
            passinfo_main.pSubpasses = &subpass_main;
            passinfo_main.dependencyCount = dependencies_main.size();
            passinfo_main.pDependencies = dependencies_main.data();

            VkRenderPass pass_main;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo_main, nullptr, &pass_main) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }

            main_pass = new RenderPass(pass_main);
            RenderPass::SetActive("main", main_pass);

            //============================SHADOW FRAME BUFFER===================//

            AttachmentReferencePasser passer(attachments_shadow);
            passer.PassView("shadowmap", shadow_imageview->GetData());


            shadow_buffer = new Framebuffer(
                shadow_map_resolution,
                shadow_map_resolution,
                shadow_pass,
                passer
                );
        }
    };
}
