#pragma once

namespace gbe::gfx {
    class DrawCall;

    struct CallInstance {
        Matrix4 model;
        DrawCall* drawcall;
        ShaderData* shaderdata;
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
        std::vector<UniformTexture> uniformTextures;
        vulkan::DescriptorPool* descriptorPool;
        std::vector<std::map<unsigned int, VkDescriptorSet>> allocdescriptorSets_perframe;

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
        inline bool ApplyOverride(const T& valueref, std::string target, unsigned int frameindex) const {
            ShaderData::ShaderBlock blockinfo;
            ShaderData::ShaderField fieldinfo;

            if (shaderdata->FindUniformField(target, fieldinfo, blockinfo) == false)
                return false;

            UniformBlockBuffer blockbuffer;

            if (this->GetBlock(blockinfo.name, blockbuffer) == false)
                return false;

            const auto& blockaddr = reinterpret_cast<char*>(blockbuffer.uboMappedPerFrame[frameindex]);
            const auto& finaladdr = blockaddr + fieldinfo.offset;

            const auto& sizeofvalue = sizeof(valueref);

            if (sizeofvalue > fieldinfo.size)
                throw std::runtime_error("Input size is larger than destination size.");

            memcpy(finaladdr, &valueref, sizeofvalue);

            return true;
        }
    };
}