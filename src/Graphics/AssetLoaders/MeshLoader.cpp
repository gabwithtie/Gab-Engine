#include "MeshLoader.h"
#include "../RenderPipeline.h" 

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mikktspace.h>

using namespace gbe::asset::data;

struct MikkUserData {
    aiMesh* mesh;
    std::vector<gbe::asset::data::Vertex>& outVertices;
};

// --- MikkTSpace Callbacks ---

int getNumFaces(const SMikkTSpaceContext* context) {
    auto data = static_cast<MikkUserData*>(context->m_pUserData);
    return data->mesh->mNumFaces;
}

int getNumVerticesOfFace(const SMikkTSpaceContext* context, int iFace) {
    return 3;
}

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
    int vertexIndex = iFace * 3 + iVert;
    data->outVertices[vertexIndex].tangent.x = tangent[0];
    data->outVertices[vertexIndex].tangent.y = tangent[1];
    data->outVertices[vertexIndex].tangent.z = tangent[2];
    data->outVertices[vertexIndex].tangent.w = fSign;
}

bgfx::VertexLayout gbe::gfx::s_VERTEXLAYOUT;

void gbe::gfx::MeshLoader::AssignSelfAsLoader()
{
    AssetLoader::AssignSelfAsLoader();
    bgfx::VertexLayout newlayout;
    newlayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
        .end();
    s_VERTEXLAYOUT = newlayout;
}

// --- STATIC WORKER FUNCTION (CPU INTENSIVE) ---
static void ProcessMeshAsync(gbe::MeshLoader::AsyncMeshTask* task)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(task->path.c_str(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        task->isDone = true; // Mark as done so the loader can handle the failure
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];
    std::vector<Vertex> flat_vertices(mesh->mNumFaces * 3);

    // 1. Flatten Vertices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];

        // Ensure face is actually a triangle (aiProcess_Triangulate should ensure this)
        if (face.mNumIndices != 3) continue;

        for (unsigned int j = 0; j < 3; j++) {
            unsigned int assimp_idx = face.mIndices[j];
            Vertex& v = flat_vertices[i * 3 + j];
            v.pos = { mesh->mVertices[assimp_idx].x, mesh->mVertices[assimp_idx].y, mesh->mVertices[assimp_idx].z };
            v.normal = { mesh->mNormals[assimp_idx].x, mesh->mNormals[assimp_idx].y, mesh->mNormals[assimp_idx].z };
            if (mesh->HasTextureCoords(0))
                v.texCoord = { mesh->mTextureCoords[0][assimp_idx].x, mesh->mTextureCoords[0][assimp_idx].y };
            if (mesh->HasVertexColors(0))
                v.color = { mesh->mColors[0][assimp_idx].r, mesh->mColors[0][assimp_idx].g, mesh->mColors[0][assimp_idx].b };
            else
                v.color = { 1.0f, 1.0f, 1.0f };
        }
    }

    // 2. MikkTSpace Generation
    MikkUserData user_data = { mesh, flat_vertices };
    SMikkTSpaceInterface interface = {};
    interface.m_getNumFaces = getNumFaces;
    interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    interface.m_getPosition = getPosition;
    interface.m_getNormal = getNormal;
    interface.m_getTexCoord = getTexCoord;
    interface.m_setTSpaceBasic = setTSpaceBasic;
    
    SMikkTSpaceContext context = { &interface, &user_data };
    genTangSpaceDefault(&context);

    // 3. Deduplication
    std::map<Vertex, uint16_t> unique_vertices;
    for (size_t i = 0; i < flat_vertices.size(); i += 3) {
        std::vector<uint16_t> face_indices;
        for (size_t j = 0; j < 3; ++j) {
            const Vertex& v = flat_vertices[i + j];
            if (unique_vertices.find(v) == unique_vertices.end()) {
                unique_vertices[v] = static_cast<uint16_t>(task->out_vertices.size());
                task->out_vertices.push_back(v);
            }
            uint16_t idx = unique_vertices[v];
            task->out_indices.push_back(idx);
            face_indices.push_back(idx);
        }
        task->out_faces.push_back(face_indices);
    }

    task->isDone = true;
}

void gbe::gfx::MeshLoader::OnAsyncTaskCompleted(MeshLoader::AsyncLoadTask* loadtask)
{
	auto meshloadtask = static_cast<MeshLoader::AsyncMeshTask*>(loadtask);

    // VERTEX BUFFER
    const size_t vbufferSize = sizeof(meshloadtask->out_vertices[0]) * meshloadtask->out_vertices.size();

    // BGFX: Create a memory reference and create the vertex buffer
    bgfx::VertexBufferHandle vertexBufferHandle = bgfx::createVertexBuffer(
        bgfx::copy(meshloadtask->out_vertices.data(), (uint32_t)vbufferSize),
        s_VERTEXLAYOUT,
        BGFX_BUFFER_NONE
    );

    if (vertexBufferHandle.idx == bgfx::kInvalidHandle) {
        throw std::runtime_error("Failed to create bgfx Vertex Buffer");
    }

    // INDEX BUFFER
    const size_t ibufferSize = sizeof(meshloadtask->out_indices[0]) * meshloadtask->out_indices.size();

    // BGFX: Create a memory reference and create the index buffer (using 16-bit indices)
    bgfx::IndexBufferHandle indexBufferHandle = bgfx::createIndexBuffer(
        bgfx::copy(meshloadtask->out_indices.data(), (uint32_t)ibufferSize),
        BGFX_BUFFER_NONE // <--- FIX: Use NONE for 16-bit (uint16_t)
    );

    if (indexBufferHandle.idx == bgfx::kInvalidHandle) {
        throw std::runtime_error("Failed to create bgfx Index Buffer");
    }

    auto newdata = MeshData{
        MeshLoadData{
            .vertices = meshloadtask->out_vertices,
            .indices = meshloadtask->out_indices,
			.faces = meshloadtask->out_faces
        },
        vertexBufferHandle, // Pass the bgfx handle
        indexBufferHandle   // Pass the bgfx handle
    };

	RegisterExternal(meshloadtask->id, newdata);
}

gbe::gfx::MeshData gbe::gfx::MeshLoader::LoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* loaddata)
{
    auto meshpath = asset->Get_asset_filepath().parent_path() / importdata.path;

	//Block sync here until the async count is less than a threshold
    if(CheckAsynchrounousTasks() >= MaxAsyncTasks)
		std::cout << "[MeshLoader] Max async tasks reached, waiting to queue new task..." << std::endl;

    while(CheckAsynchrounousTasks() >= MaxAsyncTasks) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

    // Create the task
    AsyncMeshTask* task = new AsyncMeshTask();
    task->path = meshpath.generic_string();
    task->loaddata = *loaddata;
    task->isDone = false;
	task->id = asset->Get_assetId();

    RegisterAsyncTask(task);

    // Launch worker thread
    // In a production environment, use a ThreadPool instead of raw std::thread
    std::thread worker(&ProcessMeshAsync, task);
    worker.detach();

    // Return empty mesh data immediately
    return MeshData{};
}

// NOTE: You must implement a function called every frame on the Main Thread
// to check 'task->isDone', create BGFX buffers, and set asset->SetIsLoading(false).

void gbe::gfx::MeshLoader::UnLoadAsset_(asset::Mesh* asset, const asset::data::MeshImportData& importdata, asset::data::MeshLoadData* data)
{
    const auto& meshdata = this->GetAssetRuntimeData(asset->Get_assetId());

    if (bgfx::isValid(meshdata.vertex_vbh)) bgfx::destroy(meshdata.vertex_vbh);
    if (bgfx::isValid(meshdata.index_vbh)) bgfx::destroy(meshdata.index_vbh);
}