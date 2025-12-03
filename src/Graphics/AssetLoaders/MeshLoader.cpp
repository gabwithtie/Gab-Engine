#include "MeshLoader.h"

#include "../RenderPipeline.h"
#include "Ext/GabVulkan/Objects.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <map>

using namespace gbe::asset::data;

gbe::gfx::MeshData gbe::gfx::MeshLoader::LoadAsset_(asset::Mesh * asset, const asset::data::MeshImportData & importdata, asset::data::MeshLoadData * loaddata)
{
    auto meshpath = asset->Get_asset_filepath().parent_path() / importdata.path;

    auto pathstr = meshpath.generic_string();
    auto pathcstr = pathstr.c_str();
    
    std::vector<asset::data::Vertex> vertices = {};
    std::vector<uint16_t> indices = {};
    std::vector<std::vector<uint16_t>> faces = {};

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(pathcstr,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace| 
        aiProcess_FlipUVs
        );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	throw new std::runtime_error("Failed to load mesh");

        return MeshData{
            loaddata,
            nullptr,
            nullptr
        };
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* assimpMesh = scene->mMeshes[i];
        
        // Process Vertices, Normals, and TexCoords
        for (unsigned int v = 0; v < assimpMesh->mNumVertices; v++) {
            Vertex vertex = {};

            // Position (Mandatory)
            vertex.pos = glm::vec3(
                assimpMesh->mVertices[v].x,
                assimpMesh->mVertices[v].y,
                assimpMesh->mVertices[v].z
            );

            // Normal (Conditional)
            if (assimpMesh->HasNormals()) {
                vertex.normal = glm::vec3(
                    assimpMesh->mNormals[v].x,
                    assimpMesh->mNormals[v].y,
                    assimpMesh->mNormals[v].z
                );
            }

            // Texture Coordinates (Conditional - Assimp supports multiple sets, we use the first)
            if (assimpMesh->HasTextureCoords(0)) {
                vertex.texCoord = glm::vec2(
                    assimpMesh->mTextureCoords[0][v].x,
                    assimpMesh->mTextureCoords[0][v].y
                );
            }

            if (assimpMesh->HasTangentsAndBitangents()) {
                vertex.tangent.x = assimpMesh->mTangents[v].x;
                vertex.tangent.y = assimpMesh->mTangents[v].y;
                vertex.tangent.z = assimpMesh->mTangents[v].z;
            }

            vertices.push_back(vertex);
        }

        // Process Indices (Faces)
        if (assimpMesh->HasFaces()) {
            // Because we used aiProcess_Triangulate, we know faces are triangles (3 indices).
            for (unsigned int f = 0; f < assimpMesh->mNumFaces; f++) {
                aiFace face = assimpMesh->mFaces[f];
                // Since we used Triangulate, face.mNumIndices will always be 3.
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }
        }
    }

    //VULKAN MESH SETUP vvvvvvvvvvv
    VkDeviceSize vbufferSize = sizeof(vertices[0]) * vertices.size();

    vulkan::Buffer stagingBuffer(vbufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* vdata;
    vulkan::VirtualDevice::GetActive()->MapMemory(stagingBuffer.GetMemory(), 0, vbufferSize, 0, &vdata);
    memcpy(vdata, vertices.data(), (size_t)vbufferSize);
    vulkan::VirtualDevice::GetActive()->UnMapMemory(stagingBuffer.GetMemory());

    //MM_note: Will be freed by unload asset.

    vulkan::Buffer* vertexBuffer = new vulkan::Buffer(vbufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vulkan::Buffer::CopyBuffer(&stagingBuffer, vertexBuffer, vbufferSize);

    //INDEX BUFFER
    VkDeviceSize ibufferSize = sizeof(indices[0]) * indices.size();

    vulkan::Buffer istagingBuffer(ibufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* idata;
    vulkan::VirtualDevice::GetActive()->MapMemory(istagingBuffer.GetMemory(), 0, ibufferSize, 0, &idata);
    memcpy(idata, indices.data(), (size_t)ibufferSize);
    vulkan::VirtualDevice::GetActive()->UnMapMemory(istagingBuffer.GetMemory());

    vulkan::Buffer* indexBuffer = new vulkan::Buffer(ibufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vulkan::Buffer::CopyBuffer(&istagingBuffer, indexBuffer, ibufferSize);

    //UNIFORM BUFFER
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    //COMMITTING
    loaddata->indices = indices;
    loaddata->vertices = vertices;
    loaddata->faces = faces;

    return MeshData{
        loaddata,
        vertexBuffer,
        indexBuffer
        };
}

void gbe::gfx::MeshLoader::UnLoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data)
{
    const auto& meshdata = this->GetAssetData(asset);

    delete meshdata.vertexBuffer;
    delete meshdata.indexBuffer;
}
