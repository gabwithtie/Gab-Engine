#pragma once

#include "ShadowMap.h"

#include "../Renderer.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"

namespace gbe::vulkan {

    class DeferredRenderer : public Renderer {
        //CACHE
        uint32_t width;
        uint32_t height;
        ImageView* depthImageView;

    public:

        //=============DYNAMICALLY ALLOCATED=============
        Image* positionImage = nullptr;
        Image* normalImage = nullptr;
        Image* albedoSpecImage = nullptr;

        ImageView* positionImageView = nullptr;
        ImageView* normalImageView = nullptr;
        ImageView* albedoSpecImageView = nullptr;

        RenderPass* gBufferPass = nullptr;
        Framebuffer* gBufferFramebuffer = nullptr;

        ShadowMap* directionalShadowMap = nullptr;

        inline void CreateObjects() {
            // 1. Create G-Buffer Images
            positionImage = new Image(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            normalImage = new Image(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            albedoSpecImage = new Image(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // 2. Create Image Views
            positionImageView = new ImageView(positionImage, VK_IMAGE_ASPECT_COLOR_BIT);
            normalImageView = new ImageView(normalImage, VK_IMAGE_ASPECT_COLOR_BIT);
            albedoSpecImageView = new ImageView(albedoSpecImage, VK_IMAGE_ASPECT_COLOR_BIT);

            // 3. Create the G-Buffer Render Pass
            std::vector<VkFormat> gBufferFormats = {
                positionImage->Get_format(),
                normalImage->Get_format(),
                albedoSpecImage->Get_format()
            };
            gBufferPass = new RenderPass(positionImage->Get_format());

            // 4. Create the G-Buffer Framebuffer
            std::vector<VkImageView> attachments = {
                positionImageView->GetData(),
                normalImageView->GetData(),
                albedoSpecImageView->GetData(),
                depthImageView->GetData() // Reuse the main depth buffer
            };
            gBufferFramebuffer = new Framebuffer(width, height, gBufferPass, attachments);

            directionalShadowMap = new ShadowMap();
        }

        inline DeferredRenderer(uint32_t width, uint32_t height, ImageView* depthImageView) {
			this->width = width;
			this->height = height;
			this->depthImageView = depthImageView;

            CreateObjects();
        }

        inline void Refresh() override {
            delete gBufferFramebuffer;
            delete gBufferPass;
            delete albedoSpecImageView;
            delete normalImageView;
            delete positionImageView;
            delete albedoSpecImage;
            delete normalImage;
            delete positionImage;
            delete directionalShadowMap;

            CreateObjects();
        }

        inline ~DeferredRenderer() {
            delete gBufferFramebuffer;
            delete gBufferPass;
            delete albedoSpecImageView;
            delete normalImageView;
            delete positionImageView;
            delete albedoSpecImage;
            delete normalImage;
            delete positionImage;
            delete directionalShadowMap;
        }
    };
}
