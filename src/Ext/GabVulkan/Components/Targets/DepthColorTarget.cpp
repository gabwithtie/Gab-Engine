#include "DepthColorTarget.h"

#include "../../Utility/MemoryBarrier.h"
#include "../../Utility/DebugObjectName.h"
#include "../../Objects/Instance.h"

gbe::vulkan::DepthColorTarget::DepthColorTarget(AttachmentDictionary& dict, uint32_t x, uint32_t y, std::string id, uint32_t layercount)
{
    this->x = x;
    this->y = y;
    this->layercount = layercount;

    this->depth_img = new ImagePair(new Image(
        x, y,
        PhysicalDevice::GetActive()->GetDepthFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        layercount
    ),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)depth_img->GetView()->GetData(), id + "_Depth View");
    DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)depth_img->GetImage()->GetData(), id + "_Depth Image");

    this->color_img = new ImagePair(new Image(
        x, y,
        PhysicalDevice::GetActive()->Get_swapchainFormat().format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        layercount
    ),
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)color_img->GetView()->GetData(), id + "_Main View");
    DebugObjectName::NameVkObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)color_img->GetImage()->GetData(), id + "_Main Image");

    //References
    VkAttachmentReference colorRef = dict.GetRef("color", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkAttachmentReference depthRef = dict.GetRef("depth", VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    //=======================SUBPASS========================//
    VkSubpassDescription shadowSubpass = {};
    shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    shadowSubpass.colorAttachmentCount = 1;
    shadowSubpass.pColorAttachments = &colorRef;
    shadowSubpass.pDepthStencilAttachment = &depthRef;

    std::vector<VkSubpassDependency> dependencies = {};
    dependencies.resize(1);

    //Depth test dependency
    dependencies[0] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Render Pass Info
    VkRenderPassCreateInfo passinfo = {};
    passinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passinfo.attachmentCount = dict.GetSize();
    passinfo.pAttachments = dict.GetArrayPtr();
    passinfo.subpassCount = 1;
    passinfo.pSubpasses = &shadowSubpass;
    passinfo.dependencyCount = dependencies.size();
    passinfo.pDependencies = dependencies.data();

    VkRenderPass newpass;
    if (vkCreateRenderPass(VirtualDevice::GetActive()->GetData(), &passinfo, nullptr, &newpass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    this->renderpass = new RenderPass(newpass);
    RenderPass::SetActive(id, this->renderpass);

    for (size_t i = 0; i < layercount; i++)
    {
        depth_views.push_back(new ImageView(depth_img->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT, i));
        color_views.push_back(new ImageView(color_img->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, i));

        AttachmentReferencePasser passer(dict);
        passer.PassView("depth", depth_views[i]->GetData());
        passer.PassView("color", color_views[i]->GetData());

        this->framebuffers.push_back(new Framebuffer(
            x,
            y,
            1,
            this->renderpass,
            passer
        ));
    }
}

void gbe::vulkan::DepthColorTarget::StartPass(uint32_t drawlayer)
{
	Target::StartPass(drawlayer);

    MemoryBarrier::Insert(
        Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
        depth_img->GetImage(),
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
            .layerCount = this->layercount
        }
    );

    MemoryBarrier::Insert(
        Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
        color_img->GetImage(),
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
            .layerCount = this->layercount
        }
    );

    VkRenderPassBeginInfo passBeginInfo{};

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
    scissor.extent = { x, y };
    vkCmdSetScissor(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), 0, 1, &scissor);

    passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passBeginInfo.renderPass = this->renderpass->GetData();
    passBeginInfo.framebuffer = this->framebuffers[drawlayer]->GetData();

    passBeginInfo.renderArea.offset = { 0, 0 };
    passBeginInfo.renderArea.extent = { x, y };

    std::array<VkClearValue, 2> clearValues{};
    float clear_brightness = 1.0f;
    clearValues[0].depthStencil = { 1.0f, 0 };
    clearValues[1].color = { {clear_brightness, clear_brightness, clear_brightness, 1.0f} };

    passBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    passBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData(), &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void gbe::vulkan::DepthColorTarget::EndPass()
{
    Target::EndPass();

    vkCmdEndRenderPass(Instance::GetActive()->GetCurrentCommandBuffer()->GetData());

    MemoryBarrier::Insert(
        Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
        depth_img->GetImage(),
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
            .layerCount = this->layercount
        }
    );

    MemoryBarrier::Insert(
        Instance::GetActive()->GetCurrentCommandBuffer()->GetData(),
        color_img->GetImage(),
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
            .layerCount = this->layercount
        }
    );
}
