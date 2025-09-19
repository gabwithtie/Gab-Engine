#pragma once

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"
#include "../../Objects/SwapChain.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        uint32_t shadow_map_resolution = 512;
        uint32_t max_lights = 10;

        Image* shadow_image_array;
        ImageView* shadow_imageview;

    public:

        inline ForwardRenderer() {
            shadow_image_array = new Image(
                shadow_map_resolution, shadow_map_resolution,
                VK_FORMAT_D16_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                max_lights
            );
            shadow_imageview = new ImageView(shadow_image_array, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
        }

        inline ~ForwardRenderer() {

        }

        inline void Refresh() override {

        }

        inline void PassAttachmentReferences(AttachmentReferencePasser& attachments) override {
            attachments.PassView("shadowmap", shadow_imageview->GetData());
        }

        inline void AppendRequiredAttachments(AttachmentDictionary& attachmentdict) override {
            VkAttachmentDescription shadowmap_attachment = {};
            shadowmap_attachment.format = PhysicalDevice::GetActive()->GetDepthFormat(); // Use a suitable depth format
            shadowmap_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            shadowmap_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            shadowmap_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            shadowmap_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            shadowmap_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            shadowmap_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            shadowmap_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentdict.AddAttachment("shadowmap", shadowmap_attachment);

            VkAttachmentDescription depth_attachment = {};
            depth_attachment.format = PhysicalDevice::GetActive()->GetDepthFormat();
            depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachmentdict.AddAttachment("depth", depth_attachment);

            VkAttachmentDescription color_attachment = {};
            color_attachment.format = PhysicalDevice::GetActive()->Get_swapchainFormat().format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            attachmentdict.AddAttachment("color", color_attachment);
        }

        inline RenderPass* CreateRenderPass(AttachmentDictionary& attachmentdict) override {
            // Now, let's define the subpasses.
            VkSubpassDescription subpasses[2];

            // Subpass 0: Shadow pass
            VkAttachmentReference shadowMapDepthRef = attachmentdict.GetRef("shadowmap", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpasses[0].inputAttachmentCount = 0;
            subpasses[0].pInputAttachments = nullptr;
            subpasses[0].colorAttachmentCount = 0;
            subpasses[0].pColorAttachments = nullptr;
            subpasses[0].pResolveAttachments = nullptr;
            subpasses[0].pDepthStencilAttachment = &shadowMapDepthRef;
            subpasses[0].preserveAttachmentCount = 0;
            subpasses[0].pPreserveAttachments = nullptr;

            // Subpass 1: Final render pass
            VkAttachmentReference colorRef = attachmentdict.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            VkAttachmentReference shadowMapInputRef = attachmentdict.GetRef("shadowmap", VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            VkAttachmentReference finalDepthRef = attachmentdict.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpasses[1].inputAttachmentCount = 1;
            subpasses[1].pInputAttachments = &shadowMapInputRef;
            subpasses[1].colorAttachmentCount = 1;
            subpasses[1].pColorAttachments = &colorRef;
            subpasses[1].pResolveAttachments = nullptr;
            subpasses[1].pDepthStencilAttachment = &finalDepthRef; // Use the new depth attachment
            subpasses[1].preserveAttachmentCount = 0;
            subpasses[1].pPreserveAttachments = nullptr;

            // Finally, the subpass dependency.
            // This is crucial for synchronizing the two subpasses.
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = 0; // Subpass 0 is the source
            dependency.dstSubpass = 1; // Subpass 1 is the destination
            dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            // Now, create the render pass.
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 3; // Now 3 attachments
            renderPassInfo.pAttachments = attachmentdict.GetArrayPtr();
            renderPassInfo.subpassCount = 2;
            renderPassInfo.pSubpasses = subpasses;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VkRenderPass renderPass;
            if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }

            return new RenderPass(renderPass);
        }
    };
}
