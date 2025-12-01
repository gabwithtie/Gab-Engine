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
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	throw new std::runtime_error("Failed to load mesh");

        return MeshData{
            loaddata,
            nullptr,
            nullptr
        };
    }

    std::map<Vertex, uint16_t> unique_vertices;

    aiMesh* mesh = scene->mMeshes[0];

    // Iterate over each face
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face_obj = mesh->mFaces[i];

        // A temporary vector to hold the indices for the current face
        std::vector<uint16_t> current_face_indices;

        // Iterate over each index in the face
        for (unsigned int j = 0; j < face_obj.mNumIndices; j++) {
            unsigned int assimp_vertex_index = face_obj.mIndices[j];

            Vertex vertex;

            // Populate the temporary vertex object
            if (mesh->HasPositions()) {
                vertex.pos.x = mesh->mVertices[assimp_vertex_index].x;
                vertex.pos.y = mesh->mVertices[assimp_vertex_index].y;
                vertex.pos.z = mesh->mVertices[assimp_vertex_index].z;
            }

            if (mesh->HasNormals()) {
                vertex.normal.x = mesh->mNormals[assimp_vertex_index].x;
                vertex.normal.y = mesh->mNormals[assimp_vertex_index].y;
                vertex.normal.z = mesh->mNormals[assimp_vertex_index].z;
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord.x = mesh->mTextureCoords[0][assimp_vertex_index].x;
                vertex.texCoord.y = mesh->mTextureCoords[0][assimp_vertex_index].y;
            }

            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent.x = mesh->mTangents[assimp_vertex_index].x;
                vertex.tangent.y = mesh->mTangents[assimp_vertex_index].y;
                vertex.tangent.z = mesh->mTangents[assimp_vertex_index].z;
            }

            // Deduplication logic
            if (unique_vertices.find(vertex) == unique_vertices.end()) {
                unique_vertices[vertex] = static_cast<uint16_t>(vertices.size());
                vertices.push_back(vertex);
            }

            // Add the index to both the main indices vector and the current face's indices vector
            uint16_t deduplicated_index = unique_vertices[vertex];
            indices.push_back(deduplicated_index);
            current_face_indices.push_back(deduplicated_index);
        }

        // Add the completed face to the faces vector
        faces.push_back(current_face_indices);
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
