#pragma once
#include "Asset/gbe_asset.h"
#include "../AssetLoaders/ShaderLoader.h"
#include "../AssetLoaders/MeshLoader.h"

#include "Ext/GabVulkan/Objects.h"

#include <map>

#include "CallInstance.h"

namespace gbe {
    namespace gfx {
        class DrawCall {
        private:
            asset::Mesh* m_mesh;
            asset::Material* m_material;

            std::unordered_map<std::string, std::vector<int>> overrideHandledList;
            //VULKAN
			ShaderData* shaderdata;

        public:
            int order;

            struct UniformBlockBuffer {
                std::string block_name;

                std::vector<vulkan::Buffer*> uboPerFrame;
                std::vector<void*> uboMappedPerFrame;
            };

            struct UniformTexture {
                std::string texture_name;
                unsigned int array_index;
                vulkan::ImageView* imageView;
                vulkan::Sampler* sampler;
            };

            std::vector<UniformBlockBuffer> uniformBuffers;
            std::unordered_map<std::string, std::vector<UniformTexture>> uniformTextures;
            vulkan::DescriptorPool* descriptorPool;
            std::vector<std::map<unsigned int, VkDescriptorSet>> allocdescriptorSets_perframe;

            DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* shaderdata, int order);
            ~DrawCall();

            asset::Mesh* get_mesh();
            asset::Material* get_material();
            inline ShaderData* get_shaderdata() {
                return shaderdata;
            }

            bool SyncMaterialData(unsigned int frameindex);

            inline bool GetBlock(std::string name, UniformBlockBuffer& out_block) const
            {
                for (auto& block : this->uniformBuffers)
                {
                    if (block.block_name == name)
                    {
                        out_block = block;
                        return true;
                    }
                }

                return false;
            }

            template<typename T>
            inline bool ApplyOverride(const T& valueref, std::string target, unsigned int frameindex, unsigned int arrayindex = 0) const {
                ShaderData::ShaderBlock blockinfo;
                ShaderData::ShaderField fieldinfo;

                if (shaderdata->FindUniformField(target, fieldinfo, blockinfo) == false)
                    return false;

                UniformBlockBuffer blockbuffer;

                if (this->GetBlock(blockinfo.name, blockbuffer) == false)
                    return false;

                const auto& blockaddr = reinterpret_cast<char*>(blockbuffer.uboMappedPerFrame[frameindex]);
                const auto& finaladdr = blockaddr + fieldinfo.offset + (blockinfo.block_size_aligned * arrayindex);

                const auto& sizeofvalue = sizeof(valueref);

                if (sizeofvalue > fieldinfo.size)
                    throw std::runtime_error("Input size is larger than destination size.");

                memcpy(finaladdr, &valueref, sizeofvalue);

                return true;
            }

            template <>
            inline bool ApplyOverride<TextureData>(const TextureData& valueref, std::string target, unsigned int frameindex, unsigned int arrayindex) const {
                ShaderData::ShaderField fieldinfo;
                ShaderData::ShaderBlock blockinfo;
                bool found = shaderdata->FindUniformField(target, fieldinfo, blockinfo);
                if (!found)
                    return false;

                //CREATE NEW DESCRIPTOR WRITE
                VkDescriptorImageInfo imageInfo{};

                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = valueref.textureImageView->GetData();
                imageInfo.sampler = valueref.textureSampler->GetData();

                VkWriteDescriptorSet descriptorWrite{};

                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = this->allocdescriptorSets_perframe[frameindex].at(fieldinfo.set);
                descriptorWrite.dstBinding = fieldinfo.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfo;

                vkUpdateDescriptorSets(vulkan::VirtualDevice::GetActive()->GetData(), static_cast<uint32_t>(1), &descriptorWrite, 0, nullptr);

                return true;
            }
        };
    }
}