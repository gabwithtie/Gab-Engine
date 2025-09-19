#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace gbe::vulkan {
	class AttachmentDictionary {
		std::vector<VkAttachmentDescription> descriptions;
		std::unordered_map<std::string, VkAttachmentReference> references;
	public:
		inline void AddAttachment(std::string id, VkAttachmentDescription desc) {
			descriptions.push_back(desc);
			
			VkAttachmentReference newref = {};
			newref.attachment = descriptions.size() - 1;
			
			references[id] = newref;
		}

		inline uint32_t GetSize() {
			return descriptions.size();
		}

		inline VkAttachmentDescription* GetArrayPtr() {
			return descriptions.data();
		}

		inline std::unordered_map<std::string, VkAttachmentReference>& GetMap() {
			return references;
		}

		//Promote to local variable in wherever you call this
		inline VkAttachmentReference GetRef(std::string id, VkImageLayout reflayout) {
			VkAttachmentReference newref = references[id];
			newref.layout = reflayout;
			
			return newref;
		}
	};
}