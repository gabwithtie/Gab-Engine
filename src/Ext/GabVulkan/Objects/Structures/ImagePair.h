#pragma once

#include "../Image.h"
#include "../ImageView.h"

namespace gbe::vulkan {
	class ImagePair {
	private:
		Image* img;
		ImageView* view;
	public:
		inline ImagePair(Image* img, VkImageAspectFlags aspectflags, VkImageViewType viewtype = VK_IMAGE_VIEW_TYPE_2D) {
			this->img = img;
			this->view = new ImageView(img, aspectflags, viewtype);
		}

		inline Image* GetImage() {
			return img;
		}

		inline ImageView* GetView() {
			return view;
		}

		inline ~ImagePair() {
			delete img;
			delete view;
		}
	};
}