#pragma once

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"

namespace gbe::vulkan {

    class ForwardRenderer : public Renderer {
        //CACHE
        uint32_t width;
        uint32_t height;

    public:

        //=============DYNAMICALLY ALLOCATED=============

        inline DeferredRenderer(uint32_t width, uint32_t height) {
			this->width = width;
			this->height = height;

            CreateObjects();
        }

        inline void Refresh() override {
        
        }
        
        inline RenderPass* GetRenderPass() override {
            // 1. Define Attachments
// Attachment 0: Depth attachment for the shadow map array
VkAttachmentDescription attachments[2] = {};
attachments[0].format = VK_FORMAT_D16_UNORM; // Or a suitable depth format
attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

// Attachment 1: Color attachment for the final scene
attachments[1].format = swapChainImageFormat;
attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

// 2. Define Subpasses
// Subpass 0: Shadow Pass
VkAttachmentReference depthRef = {};
depthRef.attachment = 0;
depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

VkSubpassDescription subpasses[2] = {};
subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpasses[0].colorAttachmentCount = 0;
subpasses[0].pDepthStencilAttachment = &depthRef;

// Subpass 1: Final Render Pass
VkAttachmentReference colorRef = {};
colorRef.attachment = 1;
colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpasses[1].colorAttachmentCount = 1;
subpasses[1].pColorAttachments = &colorRef;

// 3. Define Subpass Dependency
VkSubpassDependency dependency = {};
dependency.srcSubpass = 0; // Shadow Pass
dependency.dstSubpass = 1; // Final Render Pass
dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // We'll read in the shader
dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

// 4. Create Render Pass
VkRenderPassCreateInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 2;
renderPassInfo.pAttachments = attachments;
renderPassInfo.subpassCount = 2;
renderPassInfo.pSubpasses = subpasses;
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;

VkRenderPass renderPass;
if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
}

return new RenderPass(renderPass);
        }

        inline ~ForwardRenderer() {
            
        }
    };
}
