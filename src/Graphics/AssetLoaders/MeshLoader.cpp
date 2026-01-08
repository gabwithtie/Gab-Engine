#include "MeshLoader.h"

// Remove Vulkan includes
#include "../RenderPipeline.h" 

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <map>

#include <mikktspace.h>

using namespace gbe::asset::data;

struct MikkUserData {
    aiMesh* mesh;
    std::vector<gbe::asset::data::Vertex>& outVertices;
};

// Helper callbacks for MikkTSpace
int getNumFaces(const SMikkTSpaceContext* context) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);
    return data->mesh->mNumFaces;
}

int getNumVerticesOfFace(const SMikkTSpaceContext* context, int iFace) {
    return 3; // We use aiProcess_Triangulate
}

// 1. Safe Position Retrieval
void getPosition(const SMikkTSpaceContext* context, float outpos[], int iFace, int iVert) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);

    // Bounds check for the face
    if (static_cast<unsigned int>(iFace) >= data->mesh->mNumFaces) return;

    aiFace& face = data->mesh->mFaces[iFace];
    // Bounds check for the vertex index within the face
    if (static_cast<unsigned int>(iVert) >= face.mNumIndices) return;

    unsigned int index = face.mIndices[iVert];

    // CRITICAL: Bounds check for the vertex array
    if (index < data->mesh->mNumVertices) {
        auto& v = data->mesh->mVertices[index];
        outpos[0] = v.x; outpos[1] = v.y; outpos[2] = v.z;
    }
}

// 2. Safe Normal Retrieval
void getNormal(const SMikkTSpaceContext* context, float outnorm[], int iFace, int iVert) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);

    if (static_cast<unsigned int>(iFace) >= data->mesh->mNumFaces) return;

    aiFace& face = data->mesh->mFaces[iFace];
    if (static_cast<unsigned int>(iVert) >= face.mNumIndices) return;

    unsigned int index = face.mIndices[iVert];

    if (index < data->mesh->mNumVertices && data->mesh->HasNormals()) {
        auto& n = data->mesh->mNormals[index];
        outnorm[0] = n.x; outnorm[1] = n.y; outnorm[2] = n.z;
    }
}

// 3. Safe UV Retrieval
void getTexCoord(const SMikkTSpaceContext* context, float outuv[], int iFace, int iVert) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);

    if (static_cast<unsigned int>(iFace) >= data->mesh->mNumFaces) return;

    aiFace& face = data->mesh->mFaces[iFace];
    if (static_cast<unsigned int>(iVert) >= face.mNumIndices) return;

    unsigned int index = face.mIndices[iVert];

    if (index < data->mesh->mNumVertices && data->mesh->HasTextureCoords(0)) {
        auto& uv = data->mesh->mTextureCoords[0][index];
        outuv[0] = uv.x; outuv[1] = uv.y;
    }
    else {
        outuv[0] = 0.0f; outuv[1] = 0.0f;
    }
}

void setTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[], float fSign, int iFace, int iVert) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);
    // MikkTSpace works on a per-vertex-per-face basis. 
    // We store this in our temporary flat vertex list.
    int vertexIndex = iFace * 3 + iVert;
    data->outVertices[vertexIndex].tangent.x = tangent[0];
    data->outVertices[vertexIndex].tangent.y = tangent[1];
    data->outVertices[vertexIndex].tangent.z = tangent[2];
    data->outVertices[vertexIndex].tangent.w = fSign; // Handedness
}

bgfx::VertexLayout gbe::gfx::s_VERTEXLAYOUT;

void gbe::gfx::MeshLoader::AssignSelfAsLoader()
{
    AssetLoader::AssignSelfAsLoader();

    bgfx::VertexLayout newlayout;

    newlayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float) // Vector3 pos
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float) // Vector3 normal
        .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
        .end();

    s_VERTEXLAYOUT = newlayout;
}

gbe::gfx::MeshData gbe::gfx::MeshLoader::LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* loaddata)
{
    auto meshpath = asset->Get_asset_filepath().parent_path() / importdata.path;

    auto pathstr = meshpath.generic_string();
    auto pathcstr = pathstr.c_str();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(pathcstr,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
    }

    if (scene->mNumMeshes == 0) {
        throw std::runtime_error("MeshLoader: Scene contains no meshes.");
    }

    aiMesh* mesh = scene->mMeshes[0];

    // --- Safety Checks ---
    if (!mesh->HasPositions()) {
        throw std::runtime_error("MeshLoader: Mesh has no vertex positions.");
    }
    if (!mesh->HasNormals()) {
        throw std::runtime_error("MeshLoader: Mesh has no normals.");
    }

    // 1. Create a flat list of vertices (required for MikkTSpace)
    std::vector<Vertex> flat_vertices(mesh->mNumFaces * 3);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];

        // Ensure face is actually a triangle (aiProcess_Triangulate should ensure this)
        if (face.mNumIndices != 3) continue;

        for (unsigned int j = 0; j < 3; j++) {
            unsigned int assimp_idx = face.mIndices[j];

            // --- CRITICAL BOUNDS CHECK ---
            if (assimp_idx >= mesh->mNumVertices) {
                throw std::out_of_range("MeshLoader: Face index " + std::to_string(assimp_idx) +
                    " is out of bounds for mNumVertices (" +
                    std::to_string(mesh->mNumVertices) + ").");
            }

            Vertex& v = flat_vertices[i * 3 + j];

            // Positions
            v.pos = { mesh->mVertices[assimp_idx].x, mesh->mVertices[assimp_idx].y, mesh->mVertices[assimp_idx].z };

            // Normals
            v.normal = { mesh->mNormals[assimp_idx].x, mesh->mNormals[assimp_idx].y, mesh->mNormals[assimp_idx].z };

            // UVs
            if (mesh->HasTextureCoords(0)) {
                v.texCoord = { mesh->mTextureCoords[0][assimp_idx].x, mesh->mTextureCoords[0][assimp_idx].y };
            }
            else {
                v.texCoord = { 0.0f, 0.0f };
            }

            // Colors
            if (mesh->HasVertexColors(0)) {
                v.color = { mesh->mColors[0][assimp_idx].r, mesh->mColors[0][assimp_idx].g, mesh->mColors[0][assimp_idx].b };
            }
            else {
                v.color = { 1.0f, 1.0f, 1.0f };
            }
        }
    }

    // 2. MikkTSpace Context
    MikkUserData user_data = { mesh, flat_vertices };
    SMikkTSpaceInterface interface = {};
    interface.m_getNumFaces = getNumFaces;
    interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    interface.m_getPosition = getPosition;
    interface.m_getNormal = getNormal;
    interface.m_getTexCoord = getTexCoord;
    interface.m_setTSpaceBasic = setTSpaceBasic;

    SMikkTSpaceContext context = {};
    context.m_pInterface = &interface;
    context.m_pUserData = &user_data;

    if (!genTangSpaceDefault(&context)) {
        std::cerr << "Warning: MikkTSpace failed to generate tangents for " << pathstr << std::endl;
    }

    // 3. Deduplicate and generate 'indices' and 'faces'
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<std::vector<uint16_t>> faces_list;
    std::map<Vertex, uint16_t> unique_vertices;

    for (size_t i = 0; i < flat_vertices.size(); i += 3) {
        std::vector<uint16_t> current_face_indices;

        for (size_t j = 0; j < 3; ++j) {
            const Vertex& v = flat_vertices[i + j];

            if (unique_vertices.find(v) == unique_vertices.end()) {
                unique_vertices[v] = static_cast<uint16_t>(vertices.size());
                vertices.push_back(v);
            }

            uint16_t deduplicated_index = unique_vertices[v];
            indices.push_back(deduplicated_index);
            current_face_indices.push_back(deduplicated_index);
        }
        faces_list.push_back(current_face_indices);
    }

    //===================BGFX MESH SETUP===================

    // VERTEX BUFFER
    const size_t vbufferSize = sizeof(vertices[0]) * vertices.size();

    // BGFX: Create a memory reference and create the vertex buffer
    bgfx::VertexBufferHandle vertexBufferHandle = bgfx::createVertexBuffer(
        bgfx::copy(vertices.data(), (uint32_t)vbufferSize),
        s_VERTEXLAYOUT,
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
    loaddata->faces = faces_list;

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