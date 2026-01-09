#pragma once

#include <stdexcept>
#include <iostream>
#include <string>

#include <vulkan/vulkan.h>

#include "../Objects/VirtualDevice.h"

namespace gbe::vulkan {
    class DebugObjectName {
        static PFN_vkSetDebugUtilsObjectNameEXT PFN_SETDEBUGUTILSOBJECTNAME;

    public:
        static inline void Init(PFN_vkSetDebugUtilsObjectNameEXT procaddr) {
			PFN_SETDEBUGUTILSOBJECTNAME = procaddr;
        }

        static inline void NameVkObject(VkObjectType objtype, uint64_t objhandle, std::string pname) {
            if (PFN_SETDEBUGUTILSOBJECTNAME) {
                VkDebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                nameInfo.objectType = objtype;
                nameInfo.objectHandle = objhandle;
                nameInfo.pObjectName = pname.c_str();

                PFN_SETDEBUGUTILSOBJECTNAME(VirtualDevice::GetActive()->GetData(), &nameInfo);
            }
        }
    };
}