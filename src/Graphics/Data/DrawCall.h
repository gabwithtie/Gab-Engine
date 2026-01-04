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

            std::unordered_map<std::string, bool> overrideHandledList;

            // BGFX: Stores the shader program and uniform handles
            ShaderData* shaderdata;

        public:
            int order;

            DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* shaderdata, int order);
            ~DrawCall(); // The destructor must now destroy bgfx handles

            asset::Mesh* get_mesh();
            asset::Material* get_material();
            inline ShaderData* get_shaderdata() {
                return shaderdata;
            }

            // SyncMaterialData remains the same for overriding values before a draw call
            bool SyncMaterialData();

            // BGFX: ApplyOverride for simple types (bool, float, vec, matrix)
            // It uses bgfx::setUniform to update the uniform on the active view/command buffer.
            template<typename T>
            inline bool ApplyOverride(const T& valueref, std::string target) const {
                
                bgfx::UniformHandle handle;
                    
                if (!this->shaderdata->GetUniform(handle, target))
                    this->shaderdata->RegisterUniform<T>(target); //temporary fix, we want to do this at shader loading stage

                this->shaderdata->GetUniform(handle, target);

                bgfx::setUniform(handle, &valueref, 1);

                return true;
            }

            // BGFX: ApplyOverride specialization for TextureData
            template <>
            inline bool ApplyOverride<TextureData>(const TextureData& valueref, std::string target) const {
                
                bgfx::UniformHandle handle;

                if (!this->shaderdata->GetUniform(handle, target))
                    this->shaderdata->RegisterUniform<TextureData>(target); //temporary fix, we want to do this at shader loading stage

                this->shaderdata->GetUniform(handle, target);

                bgfx::setTexture(0, handle, valueref.textureHandle);

                return true;
            }

            template<typename T>
            inline bool ApplyOverrideArray(const T* valueref, std::string target, int count) const {

                bgfx::UniformHandle handle;

                if (!this->shaderdata->GetUniformArray(handle, target))
                    this->shaderdata->RegisterUniform<T>(target, count); //temporary fix, we want to do this at shader loading stage

                this->shaderdata->GetUniformArray(handle, target);

                bgfx::setUniform(handle, valueref, count);

                return true;
            }
        };
    }
}