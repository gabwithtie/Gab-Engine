#include "MeshLoader.h"

#include "../RenderPipeline.h"
#include "Ext/GabVulkan/Objects.h"

gbe::gfx::MeshData gbe::gfx::MeshLoader::LoadAsset_(asset::Mesh * asset, const asset::data::MeshImportData & importdata, asset::data::MeshLoadData * loaddata)
{
    auto meshpath = asset->Get_asset_filepath().parent_path() / importdata.path;

    //Read mesh file here
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    auto pathstr = meshpath.generic_string();
    auto pathcstr = pathstr.c_str();
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, meshpath.string().c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<asset::data::Vertex> vertices = {};
    std::vector<uint16_t> indices = {};
    std::unordered_map<std::string, uint32_t> uniqueVertices;
    std::vector<std::vector<uint16_t>> faces = {};

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            std::vector<uint16_t> cur_face = {};

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                Vector3 pos = { vx, vy, vz };
                std::string key = pos.ToString();

                // Check if `normal_index` is zero or positive. negative = no normal data
                tinyobj::real_t nx = 0;
                tinyobj::real_t ny = 0;
                tinyobj::real_t nz = 0;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                tinyobj::real_t tx = 0;
                tinyobj::real_t ty = 0;
                if (idx.texcoord_index >= 0) {
                    tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }
                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

                asset::data::Vertex vertex{
                .pos = pos,
                .normal = {nx, ny, nz},
                .color = {1, 1, 1},
                .texCoord = {tx, ty}
                };

                if (uniqueVertices.count(key) == 0)
                {
                    uniqueVertices[key] = static_cast<uint32_t>(vertices.size());
                }
                cur_face.push_back(vertices.size());
                indices.push_back(vertices.size());
                vertices.push_back(vertex);
            }

            if (fv == 3) {
                // Get the three vertices of the current face
                asset::data::Vertex& v0 = vertices[cur_face[0]];
                asset::data::Vertex& v1 = vertices[cur_face[1]];
                asset::data::Vertex& v2 = vertices[cur_face[2]];

                // Get position and texcoord data
                Vector3 pos1 = v0.pos;
                Vector3 pos2 = v1.pos;
                Vector3 pos3 = v2.pos;

                Vector2 uv1 = v0.texCoord;
                Vector2 uv2 = v1.texCoord;
                Vector2 uv3 = v2.texCoord;

                // Calculate edge vectors in 3D and UV space
                Vector3 edge1 = pos2 - pos1;
                Vector3 edge2 = pos3 - pos1;
                Vector2 uvEdge1 = uv2 - uv1;
                Vector2 uvEdge2 = uv3 - uv1;

                float determinant = (uvEdge1.x * uvEdge2.y) - (uvEdge2.x * uvEdge1.y);

                // If determinant is close to zero, it means the UVs are degenerate.
                if (abs(determinant) > 0.0001f) {
                    float invDet = 1.0f / determinant;

                    // Calculate tangent
                    Vector3 tangent;
                    tangent.x = invDet * (uvEdge2.y * edge1.x - uvEdge1.y * edge2.x);
                    tangent.y = invDet * (uvEdge2.y * edge1.y - uvEdge1.y * edge2.y);
                    tangent.z = invDet * (uvEdge2.y * edge1.z - uvEdge1.y * edge2.z);

                    // Assign tangent to all three vertices
                    v0.tangent = tangent;
                    v1.tangent = tangent;
                    v2.tangent = tangent;
                }
                else {
                    // Assign a default tangent (e.g., perpendicular to the normal)
                    v0.tangent = Vector3(1.0f, 0.0f, 0.0f);
                    v1.tangent = Vector3(1.0f, 0.0f, 0.0f);
                    v2.tangent = Vector3(1.0f, 0.0f, 0.0f);
                }
            }

            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];

            //Commit
            faces.push_back(cur_face);
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
