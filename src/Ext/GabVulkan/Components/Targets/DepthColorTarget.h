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
	
	public:
		inline ImagePair* Get_color() {
			return color_img;
		}

		inline ImagePair* Get_depth() {
			return depth_img;
		}

		DepthColorTarget(AttachmentDictionary& dict, uint32_t x, uint32_t y, std::string id);

		void StartPass() override;
		void EndPass() override;

		inline ~DepthColorTarget() {
			delete color_img;
			delete depth_img;
		}
	};
}