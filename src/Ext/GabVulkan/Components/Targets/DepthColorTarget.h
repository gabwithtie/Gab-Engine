#pragma once

#include "Target.h"

#include "../../Objects/Image.h"
#include "../../Objects/ImageView.h"
#include "../../Objects/RenderPass.h"
#include "../../Objects/Framebuffer.h"

#include "../../Objects/Structures/ImagePair.h"
#include "../../Objects/Structures/AttachmentDictionary.h"

#include <string>

namespace gbe::vulkan {
	class DepthColorTarget : public Target{
	private:
		ImagePair* color_img = nullptr;
		ImagePair* depth_img = nullptr;
	
		uint32_t layercount = 1;

		std::vector<ImageView*> color_views;
		std::vector<ImageView*> depth_views;
	public:
		inline ImagePair* Get_color() {
			return color_img;
		}

		inline ImagePair* Get_depth() {
			return depth_img;
		}

		DepthColorTarget(AttachmentDictionary& dict, uint32_t x, uint32_t y, std::string id, uint32_t layercount = 1);

		void StartPass(uint32_t drawlayer = 0) override;
		void EndPass() override;

		inline ~DepthColorTarget() {
			for (size_t i = 0; i < color_views.size(); i++)
			{
				delete color_views[i];
			}
			for (size_t i = 0; i < depth_views.size(); i++)
			{
				delete depth_views[i];
			}

			delete color_img;
			delete depth_img;
		}
	};
}