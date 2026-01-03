#include "MeshLoader.h"

// Remove Vulkan includes
#include "../RenderPipeline.h" 
// #include "Ext/GabVulkan/Objects.h" // REMOVED

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <map>

// BGFX: Define the vertex layout. This must match asset::data::Vertex.
// Note: This should ideally be defined once in your engine/pipeline setup,
// but is defined here for completeness of this file's operation.
static bgfx::VertexLayout s_vertexLayout;
static bool s_layoutInitialized = false;

using namespace gbe::asset::data;

// Helper function to ensure layout is initialized before use
static void initLayout() {
    if (!s_layoutInitialized) {
        // Assuming asset::data::Vertex is: pos(vec3), normal(vec3), texCoord(vec2), tangent(vec3)
        s_vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float) // pos
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float) // normal
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float) // texCoord
            .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float) // tangent (if used)
            .end();
        s_layoutInitialized = true;
    }
}


gbe::gfx::MeshData gbe::gfx::MeshLoader::LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* loaddata)
{
    initLayout(); // Ensure bgfx layout is set up

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

    //===================BGFX MESH SETUP===================

    // VERTEX BUFFER
    const size_t vbufferSize = sizeof(vertices[0]) * vertices.size();

    // BGFX: Create a memory reference and create the vertex buffer
    bgfx::VertexBufferHandle vertexBufferHandle = bgfx::createVertexBuffer(
        bgfx::copy(vertices.data(), (uint32_t)vbufferSize),
        s_vertexLayout,
        BGFX_BUFFER_NONE
    );

    if (vertexBufferHandle.idx == bgfx::kInvalidHandle) {
        throw std::runtime_error("Failed to create bgfx Vertex Buffer");
    }

    // INDEX BUFFER
    const size_t ibufferSize = sizeof(indices[0]) * indices.size();

    // BGFX: Create a memory reference and create the index buffer (using 16-bit indices)
    bgfx::IndexBufferHandle indexBufferHandle = bgfx::createIndexBuffer(
        bgfx::copy(indices.data(), (uint32_t)ibufferSize),
        BGFX_BUFFER_NONE // <--- FIX: Use NONE for 16-bit (uint16_t)
    );

    if (indexBufferHandle.idx == bgfx::kInvalidHandle) {
        throw std::runtime_error("Failed to create bgfx Index Buffer");
    }

    // COMMITTING
    loaddata->indices = indices;
    loaddata->vertices = vertices;
    loaddata->faces = faces;

    return MeshData{
        loaddata,
        vertexBufferHandle, // Pass the bgfx handle
        indexBufferHandle   // Pass the bgfx handle
    };
}

void gbe::gfx::MeshLoader::UnLoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data)
{
    const auto& meshdata = this->GetAssetRuntimeData(asset->Get_assetId());

    // BGFX: Destroy the handles to free GPU memory
    if (bgfx::isValid(meshdata.vertex_vbh)) {
        bgfx::destroy(meshdata.vertex_vbh);
    }
    if (bgfx::isValid(meshdata.index_vbh)) {
        bgfx::destroy(meshdata.index_vbh);
    }

    // Note: loaddata is managed by the AssetLoader base class/user
}