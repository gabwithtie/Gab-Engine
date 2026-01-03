#pragma once
#include "Asset/gbe_asset.h"
#include "../AssetLoaders/ShaderLoader.h"
#include "../AssetLoaders/MeshLoader.h"

// Replaced Vulkan includes with bgfx
#include <bgfx/bgfx.h> 

#include <map>
#include <unordered_map>
#include <stdexcept>
#include "CallInstance.h"

#include "Math/gbe_math.h" // For Vector2, Vector3, Vector4 types

namespace gbe {
    namespace gfx {

        // Forward declarations for external data types
        struct TextureData;

        class DrawCall {
        private:
            asset::Mesh* m_mesh;
            asset::Material* m_material;

            std::unordered_map<std::string, std::vector<int>> overrideHandledList;

            // BGFX: Stores the shader program and uniform handles
            ShaderData* shaderdata;

        public:
            int order;

            // BGFX: Simplified resource tracking. Uniforms are now applied via handles in shaderdata/ApplyOverride.
            // The uniformBuffers/uniformTextures structure is mostly obsolete with bgfx's model, but kept
            // minimally for conceptual compatibility or to track material texture references.
            struct UniformBlockBuffer {
                std::string block_name;
                // bgfx manages the UBOs internally; no need for per-frame buffers in this model.
            };

            struct UniformTexture {
                std::string texture_name;
                unsigned int array_index;
                bgfx::TextureHandle textureHandle = BGFX_INVALID_HANDLE; // BGFX handle for the texture
                unsigned int samplerFlags = BGFX_SAMPLER_NONE; // Optional: Sampler state flags
            };

            std::vector<UniformBlockBuffer> uniformBuffers;
            std::unordered_map<std::string, std::vector<UniformTexture>> uniformTextures;

            // bgfx manages descriptor sets/pools internally, so these are removed.
            // vulkan::DescriptorPool* descriptorPool; 
            // std::vector<std::map<unsigned int, VkDescriptorSet>> allocdescriptorSets_perframe;

            DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* shaderdata, int order);
            ~DrawCall(); // The destructor must now destroy bgfx handles

            asset::Mesh* get_mesh();
            asset::Material* get_material();
            inline ShaderData* get_shaderdata() {
                return shaderdata;
            }

            // SyncMaterialData remains the same for overriding values before a draw call
            bool SyncMaterialData(unsigned int frameindex);

            // GetBlock is simplified, as we no longer manage per-frame UBOs ourselves
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

            // BGFX: ApplyOverride for simple types (bool, float, vec, matrix)
            // It uses bgfx::setUniform to update the uniform on the active view/command buffer.
            template<typename T>
            inline bool ApplyOverride(const T& valueref, std::string target, unsigned int frameindex, unsigned int arrayindex = 0) const {
                
                bgfx::UniformHandle handle;
                    
                if (!this->shaderdata->GetUniform(handle, target))
                    this->shaderdata->SetUniform<T>(target); //temporary fix, we want to do this at shader loading stage

                this->shaderdata->GetUniform(handle, target);

                bgfx::setUniform(handle, &valueref, 1);

                return true;
            }

            // BGFX: ApplyOverride specialization for TextureData
            template <>
            inline bool ApplyOverride<TextureData>(const TextureData& valueref, std::string target, unsigned int frameindex, unsigned int arrayindex) const {
                
                bgfx::UniformHandle handle;

                if (!this->shaderdata->GetUniform(handle, target))
                    this->shaderdata->SetUniform<TextureData>(target); //temporary fix, we want to do this at shader loading stage

                this->shaderdata->GetUniform(handle, target);

                bgfx::setTexture(0, handle, valueref.textureHandle);

                return true;
            }
        };
    }
}