#include "DrawCall.h"

// Update includes
#include "../RenderPipeline.h"
#include "CallInstance.h"
#include "../AssetLoaders/TextureLoader.h" 
#include "Math/gbe_math.h" // For Vector2, Vector3, Vector4 types

namespace gbe {
    using namespace gfx;

    DrawCall::DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* _shaderdata, int order)
    {
        this->order = order;
        this->shaderdata = _shaderdata;
        this->m_mesh = mesh;
        this->m_material = material;
    }

    // BGFX: Destructor simplified. No per-frame buffers to delete.
    gfx::DrawCall::~DrawCall()
    {
        // bgfx::BufferHandles for uniforms are managed by the ShaderData/RenderPipeline, not per DrawCall UBOs.
        // We only need to clean up resources we explicitly created.
        // In this adapted model, uniformBuffers were mostly for tracking and don't hold BGFX handles to destroy.
    }

    asset::Mesh* gfx::DrawCall::get_mesh()
    {
        return this->m_mesh;
    }

    asset::Material* gfx::DrawCall::get_material()
    {
        return this->m_material;
    }

    bool gfx::DrawCall::SyncMaterialData()
    {
        for (size_t m_i = 0; m_i < this->get_material()->getOverrideCount(); m_i++)
        {
            std::string id;
            auto& overridedata = this->get_material()->getOverride(m_i, id);

            if (overridedata.registered_change == false)
            {
                overrideHandledList.insert_or_assign(id, false); //reset handled list for this id
            }

            // BGFX: ApplyOverride template handles setting the uniform state.
            // Note: The original INT override used value_float, which is potentially a bug in the original code,
            // but is preserved here by casting/passing a float to the int override.
            if (overridedata.type == asset::Shader::UniformFieldType::BOOL) {
                // BOOL is typically represented as a float/int in shaders
                this->ApplyOverride<float>(overridedata.value_bool ? 1.0f : 0.0f, id);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::FLOAT) {
                this->ApplyOverride<float>(overridedata.value_float, id);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::INT) {
                this->ApplyOverride<int>((int)overridedata.value_float, id); // Cast float to int
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC2) {
                this->ApplyOverride<Vector2>(overridedata.value_vec2, id);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC3) {
                this->ApplyOverride<Vector3>(overridedata.value_vec3, id);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC4) {
                this->ApplyOverride<Vector4>(overridedata.value_vec4, id);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::TEXTURE)
            {
                // TextureData must now be adapted to hold a bgfx::TextureHandle
                auto findtexturedata = TextureLoader::GetAssetRuntimeData(overridedata.value_tex->Get_assetId());
                this->ApplyOverride(findtexturedata, id);
            }

            overrideHandledList[id] = true;
            overridedata.registered_change = true;
        }

        return true;
    }
}