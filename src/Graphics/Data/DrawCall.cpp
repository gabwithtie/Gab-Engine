#include "DrawCall.h"

#include "../RenderPipeline.h"
#include "CallInstance.h"

namespace gbe {
    using namespace gfx;

    DrawCall::DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* _shaderdata, int order)
    {
        this->order = order;
        this->shaderdata = _shaderdata;
        this->m_mesh = mesh;
        this->m_material = material;
    }
    gfx::DrawCall::~DrawCall()
    {
        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            for (size_t j = 0; j < uniformBuffers[i].uboPerFrame.size(); j++)
            {
                vulkan::VirtualDevice::GetActive()->UnMapMemory(uniformBuffers[i].uboPerFrame[j]->GetMemory());
                delete uniformBuffers[i].uboPerFrame[j];
            }
        }
    }

    asset::Mesh* gfx::DrawCall::get_mesh()
    {
        return this->m_mesh;
    }

    asset::Material* gfx::DrawCall::get_material()
    {
        return this->m_material;
    }

    bool gfx::DrawCall::SyncMaterialData(unsigned int frameindex)
    {
        for (size_t m_i = 0; m_i < this->get_material()->getOverrideCount(); m_i++)
        {
            std::string id;
            auto& overridedata = this->get_material()->getOverride(m_i, id);

            if (overridedata.registered_change == false)
            {
				overrideHandledList.insert_or_assign(id, std::vector<int>()); //reset handled list for this id
            }
            else {
                for (const auto& frame_handled : overrideHandledList[id])
                {
					if (frame_handled == frameindex) //already handled this frame
                        continue;
                }
            }

            //CONFIRM UNIFORM FIELD EXISTS
            ShaderData::ShaderField fieldinfo;
            ShaderData::ShaderBlock blockinfo;
            bool found = shaderdata->FindUniformField(id, fieldinfo, blockinfo);
            if (!found)
                continue;

            if (overridedata.type == asset::Shader::UniformFieldType::BOOL) {
                this->ApplyOverride<bool>(overridedata.value_bool, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::FLOAT) {
                this->ApplyOverride<float>(overridedata.value_float, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::INT) {
                this->ApplyOverride<int>(overridedata.value_float, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC2) {
                this->ApplyOverride<Vector2>(overridedata.value_vec2, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC3) {
                this->ApplyOverride<Vector3>(overridedata.value_vec3, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC4) {
                this->ApplyOverride<Vector4>(overridedata.value_vec4, id, frameindex);
            }
            else if (overridedata.type == asset::Shader::UniformFieldType::TEXTURE)
            {
                auto findtexturedata = TextureLoader::GetAssetData(overridedata.value_tex);

                this->ApplyOverride(findtexturedata, id, frameindex);
            }

			overrideHandledList[id].push_back(frameindex);

            overridedata.registered_change = true;
        }

        return true;
    }
}
