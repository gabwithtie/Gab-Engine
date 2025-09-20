#pragma once

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"
#include "../../Objects/SwapChain.h"
#include "../../Objects/Instance.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        uint32_t shadow_map_resolution = 512;
        uint32_t max_lights = 10;

        Image* shadow_image_array = nullptr;
        ImageView* shadow_imageview = nullptr;
        Framebuffer* shadow_buffer = nullptr;

        AttachmentDictionary attachments_shadow;

        RenderPass* shadow_pass = nullptr;

    public:

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
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
        }

        inline ~ForwardRenderer() {

        }

        inline void StartShadowPass() {
            VkRenderPassBeginInfo shadowPassBeginInfo{};
            shadowPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            shadowPassBeginInfo.renderPass = shadow_pass->GetData();
            shadowPassBeginInfo.framebuffer = shadow_buffer->GetData();

            shadowPassBeginInfo.renderArea.offset = { 0, 0 };
            shadowPassBeginInfo.renderArea.extent = {shadow_map_resolution, shadow_map_resolution};

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            shadowPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            shadowPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), &shadowPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        inline void TransitionToMainPass() {
            vkCmdEndRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData());

            VkRenderPassBeginInfo mainPassBeginInfo{};
            mainPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            mainPassBeginInfo.renderPass = main_pass->GetData();
            mainPassBeginInfo.framebuffer = Instance::GetActive()->GetCurrentSwapchainBuffer()->GetData();

            mainPassBeginInfo.renderArea.offset = { 0, 0 };
            mainPassBeginInfo.renderArea.extent = SwapChain::GetActive()->GetExtent();

            std::array<VkClearValue, 2> clearValues{};
            float clear_brightness = 0.3f;
            clearValues[0].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

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
            dependencies_main[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies_main[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies_main[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            // Now, create the render pass.
            VkRenderPassCreateInfo passinfo_main = {};
            passinfo_main.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            passinfo_main.attachmentCount = attachments_main.GetSize(); // Now 3 attachments
            passinfo_main.pAttachments = attachments_main.GetArrayPtr();
            passinfo_main.subpassCount = 1;
            passinfo_main.pSubpasses = &subpass_main;
            passinfo_main.dependencyCount = dependencies_main.size();
            passinfo_main.pDependencies = dependencies_main.data();

            VkRenderPass pass_main;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo_main, nullptr, &pass_main) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }

            shadow_pass = new RenderPass(pass_shadow);
            main_pass = new RenderPass(pass_main);

            //============================SHADOW FRAME BUFFER===================//

            VkImageView attachments[] = {
                shadow_imageview->GetData() // The image view for your 2D array texture
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = shadow_pass->GetData(); // Use the new shadow render pass
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = shadow_map_resolution; // Use the shadow map's width
            framebufferInfo.height = shadow_map_resolution; // Use the shadow map's height
            framebufferInfo.layers = 1;

            VkFramebuffer shadowMapFramebuffer;
            vkCreateFramebuffer(VirtualDevice::GetActive()->GetData(), &framebufferInfo, nullptr, &shadowMapFramebuffer);
        }
    };
}
