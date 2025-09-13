#include "DrawCall.h"

#include "../RenderPipeline.h"
#include "CallInstance.h"

namespace gbe {
    using namespace gfx;

    DrawCall::DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* _shaderdata, unsigned int MAX_FRAMES_IN_FLIGHT)
    {
        this->shaderdata = _shaderdata;
        this->m_mesh = mesh;
        this->m_material = material;
        this->MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
    }
    gfx::DrawCall::~DrawCall()
    {
    }

    asset::Mesh* gfx::DrawCall::get_mesh()
    {
        return this->m_mesh;
    }

    asset::Material* gfx::DrawCall::get_material()
    {
        return this->m_material;
    }

    bool gfx::DrawCall::SyncMaterialData(unsigned int frameindex, const CallInstance& callinst)
    {
        if(callinst.drawcall != this)
			throw new std::runtime_error("CallInstance does not belong to this DrawCall!");

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

            if (overridedata.type == asset::Shader::UniformFieldType::TEXTURE)
            {
                auto findtexturedata = TextureLoader::GetAssetData(overridedata.value_tex);

                //CREATE NEW DESCRIPTOR WRITE
                VkDescriptorImageInfo imageInfo{};

                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = findtexturedata.textureImageView->GetData();
                imageInfo.sampler = findtexturedata.textureSampler->GetData();

                VkWriteDescriptorSet descriptorWrite{};

                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = callinst.allocdescriptorSets_perframe[frameindex].at(fieldinfo.set);
                descriptorWrite.dstBinding = fieldinfo.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfo;

                vkUpdateDescriptorSets(vulkan::VirtualDevice::GetActive()->GetData(), static_cast<uint32_t>(1), &descriptorWrite, 0, nullptr);

            }
            else if (overridedata.type == asset::Shader::UniformFieldType::VEC4) {
                callinst.ApplyOverride<Vector4>(overridedata.value_vec4, id, frameindex);
            }

			overrideHandledList[id].push_back(frameindex);

            overridedata.registered_change = true;
        }

        return true;
    }
}
