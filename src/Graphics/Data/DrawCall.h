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
            unsigned int MAX_FRAMES_IN_FLIGHT = 2;
        public:

            DrawCall(asset::Mesh* mesh, asset::Material* material, ShaderData* shaderdata, unsigned int MAX_FRAMES_IN_FLIGHT);
            ~DrawCall();

            asset::Mesh* get_mesh();
            asset::Material* get_material();
            inline ShaderData* get_shaderdata() {
                return shaderdata;
            }

            bool SyncMaterialData(unsigned int frameindex, const CallInstance& callinst);
        };
    }
}