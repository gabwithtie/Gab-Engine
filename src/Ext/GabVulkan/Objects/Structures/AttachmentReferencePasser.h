#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

#include "AttachmentDictionary.h"

namespace gbe::vulkan {
	class AttachmentReferencePasser {
		AttachmentDictionary& dict;

		std::unordered_map<std::string, VkImageView*> map;
		std::vector<VkImageView> data;

	public:
		inline AttachmentReferencePasser(AttachmentDictionary& _dict) :
			dict(_dict) {
			
			for (const auto& pair : dict.GetMap())
			{
				this->map[pair.first] = nullptr;
			}
		}

		inline void PassView(std::string id, VkImageView view) {
			auto has = map.find(id) != map.end();

			if (!has)
				throw new std::runtime_error("Trying to pass unrequired view.");
			
			if (map[id] != nullptr)
				*map[id] = view;
			else
				data.push_back(view);

			map[id] = &data[data.size() - 1];
		}

		inline uint32_t GetSize() {
			return data.size();
		}

		inline VkImageView* TryGetPasserPtr() {
			for (const auto& pair : map)
			{
				if (pair.second == nullptr)
					throw new std::runtime_error("Trying to pass incomplete attachment array.");
			}

			return data.data();
		}
	};
}