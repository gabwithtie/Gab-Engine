#pragma once

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"
#include "../../Objects/SwapChain.h"
#include "../../Objects/Instance.h"
#include "../../Objects/Sampler.h"
#include "../../Objects/Structures/ImagePair.h"

#include "../../Utility/MemoryBarrier.h"

#include "../../Utility/DebugObjectName.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        uint32_t shadow_map_resolution = 1028;
        uint32_t max_lights = 1;

        Sampler shadowmap_sampler;
        AttachmentDictionary attachments_shadow;

        ImagePair* sp_main = nullptr;
        ImagePair* sp_depth = nullptr;
        
        Framebuffer* shadow_buffer = nullptr;

        ImagePair* main_depth = nullptr;

        std::vector<ImageView*> sp_imageview_layers;

        RenderPass* shadow_pass = nullptr;

    public:
        inline ImageView* Get_shadowmap_layer(
            uint32_t index
        ) {
            return sp_imageview_layers[index];
        }

        inline ImageView* Get_sp_view() {
            return sp_main->GetView();
        }

        inline uint32_t Get_max_lights()
        {
            return max_lights;
        }

        inline Sampler* Get_shadowmap_sampler() {
            return &shadowmap_sampler;
        }

        inline ForwardRenderer() {
            sp_depth = new ImagePair(new Image(
                shadow_map_resolution, shadow_map_resolution,
                PhysicalDevice::GetActive()->GetDepthFormat(),
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                max_lights
            ),
                VK_IMAGE_ASPECT_DEPTH_BIT
            );

            DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)sp_depth->GetView()->GetData(), "Shadow Pass Depth View");
            DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)sp_depth->GetImage()->GetData(), "Shadow Pass Depth Image");

            sp_main = new ImagePair(new Image(
                shadow_map_resolution, shadow_map_resolution,
                PhysicalDevice::GetActive()->Get_swapchainFormat().format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                max_lights
            ),
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)sp_main->GetView()->GetData(), "Shadow Pass Main View");
            DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)sp_main->GetImage()->GetData(), "Shadow Pass Main Image");


            for (size_t i = 0; i < max_lights; i++)
            {
                sp_imageview_layers.push_back(new ImageView(sp_depth->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, i));
            }
            
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
            attachments_shadow.AddAttachment("depth", depth_attachment);

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
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments_shadow.AddAttachment("color", color_attachment);
        }

        inline ~ForwardRenderer() {
            delete sp_depth;
            delete sp_main;
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

        inline void PassAttachments(AttachmentReferencePasser& newpasser) override
        {
            main_depth->GetImage()->transitionImageLayout(this->attachments_main.GetAttachmentDesc("depth").finalLayout, VK_IMAGE_ASPECT_DEPTH_BIT);
            newpasser.PassView("depth", main_depth->GetView()->GetData());
        }

        inline void StartShadowPass() {
            MemoryBarrier::Insert(
                Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
                sp_depth->GetImage(),
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = max_lights
                }
            );

            MemoryBarrier::Insert(
                Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
                sp_main->GetImage(),
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = max_lights
                }
            );

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

            MemoryBarrier::Insert(
                Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
                sp_depth->GetImage(),
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

            MemoryBarrier::Insert(
                Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
                sp_main->GetImage(),
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = max_lights
                }
            );

            SetBounds(
                vulkan::SwapChain::GetActive()->GetExtent().width,
                vulkan::SwapChain::GetActive()->GetExtent().height
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

        inline void Refresh(uint32_t x, uint32_t y) override {
            if (shadow_buffer != nullptr)
                delete shadow_buffer;
            if (shadow_pass != nullptr)
                delete shadow_pass;
            if (main_pass != nullptr)
                delete main_pass;

            auto depthformat = PhysicalDevice::GetActive()->GetDepthFormat();
            main_depth = new ImagePair(
                new Image(x, y, depthformat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                VK_IMAGE_ASPECT_DEPTH_BIT
                );

            //References
            VkAttachmentReference shadowMapColorRef = attachments_shadow.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkAttachmentReference shadowMapDepthRef = attachments_shadow.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            VkAttachmentReference mainColorRef = attachments_main.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkAttachmentReference mainDepthRef = attachments_main.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            //=======================SHADOW PASS========================//
            VkSubpassDescription shadowSubpass = {};
            shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            shadowSubpass.colorAttachmentCount = 1;
            shadowSubpass.pColorAttachments = &shadowMapColorRef;
            shadowSubpass.pDepthStencilAttachment = &shadowMapDepthRef;

            std::vector<VkSubpassDependency> dependencies_shadow = {};
            dependencies_shadow.resize(1);
            //Depth test dependency
            dependencies_shadow[0] = {};
            dependencies_shadow[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies_shadow[0].dstSubpass = 0;
            dependencies_shadow[0].srcAccessMask = 0;
            dependencies_shadow[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dependencies_shadow[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies_shadow[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


            // Render Pass Info
            VkRenderPassCreateInfo passinfo_shadow = {};
            passinfo_shadow.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            passinfo_shadow.attachmentCount = attachments_shadow.GetSize();
            passinfo_shadow.pAttachments = attachments_shadow.GetArrayPtr();
            passinfo_shadow.subpassCount = 1;
            passinfo_shadow.pSubpasses = &shadowSubpass;
            passinfo_shadow.dependencyCount = dependencies_shadow.size();
            passinfo_shadow.pDependencies = dependencies_shadow.data();

            VkRenderPass pass_shadow;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo_shadow, nullptr, &pass_shadow) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
            shadow_pass = new RenderPass(pass_shadow);
            RenderPass::SetActive("shadow", shadow_pass);

            AttachmentReferencePasser passer(attachments_shadow);
            passer.PassView("depth", sp_depth->GetView()->GetData());
            passer.PassView("color", sp_main->GetView()->GetData());

            shadow_buffer = new Framebuffer(
                shadow_map_resolution,
                shadow_map_resolution,
                shadow_pass,
                passer
            );

            //====================MAIN PASS====================//
            VkSubpassDescription subpass_main{};
            subpass_main.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_main.colorAttachmentCount = 1;
            subpass_main.pColorAttachments = &mainColorRef;
            subpass_main.pDepthStencilAttachment = &mainDepthRef;

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
        }
    };
}
