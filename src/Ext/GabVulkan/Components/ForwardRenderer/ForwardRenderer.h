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

#include "../Targets/DepthColorTarget.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        uint32_t shadow_map_resolution = 2056;
        uint32_t main_x = 1028;
        uint32_t main_y = 1028;
        const uint32_t max_lights = 5;

        Sampler render_sampler;
        AttachmentDictionary attachments_colordepth;

        //DYNAMICALLY ALLOCATED
        DepthColorTarget* shadowpass = nullptr;
        DepthColorTarget* mainpass = nullptr;

        ImagePair* screen_depth = nullptr;

        std::vector<ImageView*> sp_imageview_layers;

    public:
        inline ImageView* Get_shadowmap_layer(
            uint32_t index
        ) {
            return sp_imageview_layers[index];
        }

        inline DepthColorTarget* Get_shadowpass() {
            return shadowpass;
        }

        inline DepthColorTarget* Get_mainpass() {
            return mainpass;
        }

        inline uint32_t Get_max_lights()
        {
            return max_lights;
        }

        inline Sampler* Get_sampler() {
            return &render_sampler;
        }

        inline ForwardRenderer() :
            render_sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
        {
            //==================DICTIONARY POPULATING====================//
            VkAttachmentDescription depth_attachment = {};
            depth_attachment.format = PhysicalDevice::GetActive()->GetDepthFormat();
            depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments_screen.AddAttachment("depth", depth_attachment);
            attachments_colordepth.AddAttachment("depth", depth_attachment);

            VkAttachmentDescription color_attachment = {};
            color_attachment.format = PhysicalDevice::GetActive()->Get_swapchainFormat().format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            attachments_screen.AddAttachment("color", color_attachment);
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments_colordepth.AddAttachment("color", color_attachment);

            //=========================SHADOW MAP CACHING=======================
            shadowpass = new DepthColorTarget(attachments_colordepth, shadow_map_resolution, shadow_map_resolution, "shadow", this->max_lights);

            for (size_t i = 0; i < max_lights; i++)
            {
                sp_imageview_layers.push_back(new ImageView(shadowpass->Get_color()->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, i));
            }
        }

        inline ~ForwardRenderer() {
            delete shadowpass;
        }

        inline void PassAttachments(AttachmentReferencePasser& newpasser) override
        {
            screen_depth->GetImage()->transitionImageLayout(this->attachments_screen.GetAttachmentDesc("depth").finalLayout, VK_IMAGE_ASPECT_DEPTH_BIT);
            newpasser.PassView("depth", screen_depth->GetView()->GetData());
        }

        inline void StartShadowPass(unsigned int layerindex) {
            shadowpass->StartPass(layerindex);
        }
        inline void EndShadowPass() {
            shadowpass->EndPass();
        }

        inline void StartMainPass() {
            mainpass->StartPass();
        }

        inline void TransitionToScreenPass() {
            mainpass->EndPass();

            VkExtent2D extents = vulkan::SwapChain::GetActive()->GetExtent();

            //BOUNDS
            VkViewport viewport{};
            viewport.width = extents.width;
            viewport.height = extents.height;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = extents;
            vkCmdSetScissor(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), 0, 1, &scissor);

            //RENDER PASS START
            VkRenderPassBeginInfo screenPassBeginInfo{};
            screenPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            screenPassBeginInfo.renderPass = screen_pass->GetData();
            screenPassBeginInfo.framebuffer = Instance::GetActive()->GetCurrentSwapchainBuffer()->GetData();

            screenPassBeginInfo.renderArea.offset = { 0, 0 };
            screenPassBeginInfo.renderArea.extent = SwapChain::GetActive()->GetExtent();

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].depthStencil = { 1.0f, 0 };
            clearValues[1].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };

            screenPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            screenPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), &screenPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        inline void EndScreenPass() {
            vkCmdEndRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData());
        }

        inline void Refresh(uint32_t x, uint32_t y) override {
            this->main_x = x;
            this->main_y = y;
            
            if (mainpass != nullptr)
                delete mainpass;
            if (screen_pass != nullptr)
                delete screen_pass;

            mainpass = new DepthColorTarget(attachments_colordepth, x, y, "main");
            auto screen_extents = SwapChain::GetActive()->GetExtent();

            auto depthformat = PhysicalDevice::GetActive()->GetDepthFormat();
            screen_depth = new ImagePair(
                new Image(screen_extents.width, screen_extents.height, depthformat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                VK_IMAGE_ASPECT_DEPTH_BIT
                );

            //References
            VkAttachmentReference mainColorRef = attachments_screen.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            VkAttachmentReference mainDepthRef = attachments_screen.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

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
            passinfo_main.attachmentCount = attachments_screen.GetSize();
            passinfo_main.pAttachments = attachments_screen.GetArrayPtr();
            passinfo_main.subpassCount = 1;
            passinfo_main.pSubpasses = &subpass_main;
            passinfo_main.dependencyCount = dependencies_main.size();
            passinfo_main.pDependencies = dependencies_main.data();

            VkRenderPass pass_main;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo_main, nullptr, &pass_main) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }

            screen_pass = new RenderPass(pass_main);
            RenderPass::SetActive("screen", screen_pass);
        }
    };
}
