set(LIB_NAME "gbe_ext_gabvulkan")
message("[GBE EXTERNAL] Building Library "${LIB_NAME})

add_library(${LIB_NAME} "GabVulkan.cpp" "VulkanObjectSingleton.h" "Objects/Sampler.h" "Objects/DescriptorPool.h" "Objects/Framebuffer.h" "Objects/RenderPass.h" "Objects/Structures/FrameSyncronizationObject.h" "Utility/ValidationLayers.h" "Objects/CommandBuffer.h")

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	target_link_libraries(${LIB_NAME} PRIVATE ${Vulkan_LIBRARIES})
	target_include_directories(${LIB_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
	target_include_directories(${LIB_NAME} PRIVATE ${VULKANSDK_DIR})
	message("[GABENGINE] INCLUDED: " ${Vulkan_INCLUDE_DIRS})
	message("[GABENGINE] VULKAN SDK AT: " ${VULKANSDK_DIR})
else()
	message(FATAL_ERROR "[GABENGINE] VULKAN NOT FOUND")
endif()


target_include_directories(${LIB_NAME} PRIVATE ${GABENGINE_INCLUDE_DIRECTORY})
