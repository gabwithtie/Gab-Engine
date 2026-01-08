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

            // BGFX: Stores the shader program and uniform handles
            ShaderData* shaderdata;

        public:
            DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* shaderdata);
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
                this->shaderdata->ApplyOverride(valueref, target);
                return true;
            }

            inline bool ApplyTextureOverride(const TextureData& valueref, std::string target, int stage) const {
                this->shaderdata->ApplyTextureOverride(valueref, target, stage);
                return true;
            }

            template<typename T>
            inline bool ApplyOverrideArray(const T* valueref, std::string target, int count) const {
                this->shaderdata->ApplyOverrideArray(valueref, target, count);
                return true;
            }
        };
    }
}