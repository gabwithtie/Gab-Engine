#pragma once

#include "Lighting.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/Sampler.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"


namespace gbe::vulkan {

    class ShadowMap {
    public:
        const uint32_t SHADOW_MAP_SIZE = 2048;

        Image* shadowMapImage = nullptr;
        ImageView* shadowMapImageView = nullptr;
        Sampler* shadowMapSampler = nullptr;
        RenderPass* shadowPass = nullptr;
        Framebuffer* shadowFramebuffer = nullptr;

        ShadowCastingData shadowData;

        inline ShadowMap() {
            // 1. Create the shadow map image (depth buffer)
            auto depthFormat = PhysicalDevice::GetActive()->FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            shadowMapImage = new Image(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            shadowMapImageView = new ImageView(shadowMapImage, VK_IMAGE_ASPECT_DEPTH_BIT);

            // 2. Create a sampler to read from the shadow map in the lighting shader
            shadowMapSampler = new Sampler();

            // 3. Create a render pass for rendering just depth
            shadowPass = new RenderPass(depthFormat); // Overload for depth-only

            // 4. Create the framebuffer
            std::vector<VkImageView> attachments = {
                shadowMapImageView->GetData()
			};
            shadowFramebuffer = new Framebuffer(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, shadowPass, attachments);
        }

        inline ~ShadowMap() {
            delete shadowFramebuffer;
            delete shadowPass;
            delete shadowMapSampler;
            delete shadowMapImageView;
            delete shadowMapImage;
        }

        // Calculates the view/projection matrix from the light's perspective
        inline void UpdateLightSpaceMatrix(const gbe::vulkan::LightData& light, float sceneRadius) {
            Matrix4 lightProjection = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, -sceneRadius, sceneRadius);

            Vector3 lightPos = -light.direction * sceneRadius; // Position the light to see the whole scene
            Matrix4 lightView = glm::lookAt(lightPos, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));

            lightProjection[1][1] *= -1; // Vulkan Y-flip
            shadowData.lightSpaceMatrix = lightProjection * lightView;
        }
    };
}
